#include "AutoLoot.h"
#include "Console.h"
#include "MemoryGuard.h"
#include <map>
#include <Windows.h>

// ============================================
// AUTO LOOT STATE
// ============================================

bool g_AutoLootEnabled = false;

static Class g_ContainerWindowClass;
static Class g_ContainerClass;

static void* g_GetContainerMethod = nullptr;
static void* g_LootAllMethod = nullptr;

static std::map<void*, DWORD> g_LootCalledWindows;
static std::map<void*, DWORD> g_DoneWindows;
static std::map<void*, DWORD> g_FirstSeenWindows;

static DWORD g_LastLootAttempt = 0;
static const DWORD LOOT_COOLDOWN = 50;
static const DWORD DONE_EXPIRE = 10000;
static const DWORD ITEM_LOAD_GRACE = 3000;
static const DWORD LOOT_INITIAL_DELAY = 750;
static const DWORD LOOT_RETRY_INTERVAL = 500;
static const DWORD LOOT_GIVE_UP = 5000;

// Map size caps to prevent unbounded growth
static const size_t MAX_DONE_WINDOWS = 50;
static const size_t MAX_FIRST_SEEN = 20;
static const size_t MAX_LOOT_CALLED = 10;

// ============================================
// HOOK STATE
// ============================================

typedef void(__fastcall* NativeMethodFn)(void* thisPtr, void* methodInfo);
static NativeMethodFn g_OriginalCWUpdate = nullptr;
static void* g_CWUpdateMethodInfo = nullptr;

static int g_CWUpdateCrashCount = 0;
static const int MAX_CRASH_COUNT = 5;

static int g_TotalLooted = 0;
static int g_TotalSkippedEmpty = 0;
static int g_TotalRetries = 0;
static DWORD g_LastDiagnosticLog = 0;
static const DWORD DIAGNOSTIC_INTERVAL = 10000;
static DWORD g_LastMapTrim = 0;
static const DWORD MAP_TRIM_INTERVAL = 5000;

// ============================================
// SEH WRAPPERS
// ============================================

static int SafeSEHFilter(unsigned int code) {
    if (code == EXCEPTION_ACCESS_VIOLATION) return EXCEPTION_EXECUTE_HANDLER;
    if (code == EXCEPTION_STACK_OVERFLOW) return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

static bool SEH_ReadPtr(void* addr, void** out) {
    __try {
        if (!addr) return false;
        if (IsBadReadPtr(addr, sizeof(void*))) return false;
        *out = *(void**)addr;
        return true;
    }
    __except (SafeSEHFilter(GetExceptionCode())) {
        return false;
    }
}

static bool SEH_ReadInt(void* addr, int* out) {
    __try {
        if (!addr) return false;
        if (IsBadReadPtr(addr, sizeof(int))) return false;
        *out = *(int*)addr;
        return true;
    }
    __except (SafeSEHFilter(GetExceptionCode())) {
        return false;
    }
}

static bool SEH_IsBadRead(void* addr, size_t size) {
    __try {
        return IsBadReadPtr(addr, size) != 0;
    }
    __except (SafeSEHFilter(GetExceptionCode())) {
        return true;
    }
}

static bool SEH_CallOriginalCWUpdate(void* thisPtr, void* methodInfo) {
    __try {
        if (g_OriginalCWUpdate) g_OriginalCWUpdate(thisPtr, methodInfo);
        return true;
    }
    __except (SafeSEHFilter(GetExceptionCode())) {
        return false;
    }
}

static bool SEH_InvokeLootAll(void* lootAllMethod, void* window) {
    __try {
        void* exc = nullptr;
        Functions.runtime_invoke(lootAllMethod, window, nullptr, &exc);
        if (exc) return false;
        return true;
    }
    __except (SafeSEHFilter(GetExceptionCode())) {
        return false;
    }
}

static void* SEH_RuntimeInvoke(void* method, void* obj, void** args) {
    __try {
        void* exc = nullptr;
        void* result = Functions.runtime_invoke(method, obj, args, &exc);
        if (exc) return nullptr;
        return result;
    }
    __except (SafeSEHFilter(GetExceptionCode())) {
        return nullptr;
    }
}

// ============================================
// MAP TRIMMING — prevents unbounded growth
// ============================================

static void TrimMap(std::map<void*, DWORD>& m, size_t maxSize, DWORD now, DWORD maxAge) {
    // First pass: remove expired
    auto it = m.begin();
    while (it != m.end()) {
        if (now - it->second > maxAge)
            it = m.erase(it);
        else
            ++it;
    }
    // Second pass: if still too big, remove oldest
    while (m.size() > maxSize) {
        DWORD oldest = MAXDWORD;
        auto oldestIt = m.begin();
        for (auto check = m.begin(); check != m.end(); ++check) {
            if (check->second < oldest) {
                oldest = check->second;
                oldestIt = check;
            }
        }
        m.erase(oldestIt);
    }
}

// Called by MemoryGuard during cleanup
void AutoLootTrimMaps() {
    DWORD now = GetTickCount();
    g_DoneWindows.clear();
    g_FirstSeenWindows.clear();
    g_LootCalledWindows.clear();
}

// ============================================
// WINDOW / CONTAINER HELPERS
// ============================================

static bool IsWindowValid(void* containerWindow) {
    void* container = nullptr;
    if (!SEH_ReadPtr((void*)((uintptr_t)containerWindow + 0xC0), &container)) return false;
    return container != nullptr;
}

static void* GetContainerFromWindow(void* containerWindow) {
    if (!containerWindow) return nullptr;
    if (g_GetContainerMethod) {
        void* container = SEH_RuntimeInvoke(g_GetContainerMethod, containerWindow, nullptr);
        if (container) return container;
    }
    void* container = nullptr;
    if (SEH_ReadPtr((void*)((uintptr_t)containerWindow + 0xC0), &container))
        return container;
    return nullptr;
}

static int GetItemCount(void* container) {
    if (!container) return 0;
    void* list = nullptr;
    if (!SEH_ReadPtr((void*)((uintptr_t)container + 0xE8), &list)) return 0;
    if (!list) return 0;
    int size = 0;
    if (!SEH_ReadInt((void*)((uintptr_t)list + 0x18), &size)) return 0;
    if (size < 0 || size > 200) return 0;
    return size;
}

static bool HasLootPanel(void* containerWindow) {
    if (!containerWindow) return false;
    void* lootPanel = nullptr;
    if (!SEH_ReadPtr((void*)((uintptr_t)containerWindow + 0x88), &lootPanel)) return false;
    return lootPanel != nullptr;
}

static bool HasContainerDetailsReceived(void* container) {
    if (!container) return false;
    __try {
        if (IsBadReadPtr((void*)((uintptr_t)container + 0xF0), sizeof(bool))) return false;
        return *(bool*)((uintptr_t)container + 0xF0);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// ============================================
// HOOKED: ContainerWindow.Update
// ============================================

static void __fastcall HookedContainerWindowUpdate(void* thisPtr, void* methodInfo) {
    if (!SEH_CallOriginalCWUpdate(thisPtr, methodInfo)) {
        g_CWUpdateCrashCount++;
        if (g_CWUpdateCrashCount <= 3) {
            char msg[128]; sprintf_s(msg, "[AL] SEH in CW.Update! (#%d)", g_CWUpdateCrashCount);
            LogError(msg);
        }
        return;
    }

    if (!g_AutoLootEnabled) return;
    if (g_CWUpdateCrashCount >= MAX_CRASH_COUNT) return;
    if (!g_LootAllMethod) return;
    if (!IsMemoryFeatureSafe()) return;

    DWORD now = GetTickCount();

    // Periodic map trim
    if (now - g_LastMapTrim >= MAP_TRIM_INTERVAL) {
        g_LastMapTrim = now;
        TrimMap(g_DoneWindows, MAX_DONE_WINDOWS, now, DONE_EXPIRE);
        TrimMap(g_FirstSeenWindows, MAX_FIRST_SEEN, now, ITEM_LOAD_GRACE + LOOT_INITIAL_DELAY + 1000);
        TrimMap(g_LootCalledWindows, MAX_LOOT_CALLED, now, LOOT_GIVE_UP + 1000);
    }

    // Diagnostics
    if (now - g_LastDiagnosticLog >= DIAGNOSTIC_INTERVAL) {
        g_LastDiagnosticLog = now;
        char diagMsg[256];
        sprintf_s(diagMsg, "[AL] DIAG: looted=%d skipped=%d retries=%d crash=%d maps=%d/%d/%d mem=%luMB",
            g_TotalLooted, g_TotalSkippedEmpty, g_TotalRetries,
            g_CWUpdateCrashCount,
            (int)g_LootCalledWindows.size(), (int)g_DoneWindows.size(), (int)g_FirstSeenWindows.size(),
            GetWorkingSetMB());
        LogInfo(diagMsg);

        if (g_CWUpdateCrashCount >= MAX_CRASH_COUNT) {
            LogError("[AL] Too many crashes — disabling!");
            g_AutoLootEnabled = false;
            return;
        }
    }

    // ===== PHASE 1: Already done? =====
    auto doneIt = g_DoneWindows.find(thisPtr);
    if (doneIt != g_DoneWindows.end()) {
        if (now - doneIt->second > DONE_EXPIRE)
            g_DoneWindows.erase(doneIt);
        else
            return;
    }

    if (!HasLootPanel(thisPtr)) return;
    if (!IsWindowValid(thisPtr)) return;

    void* container = GetContainerFromWindow(thisPtr);
    if (!container) return;

    // ===== PHASE 2: LootAll already called? Monitor =====
    auto calledIt = g_LootCalledWindows.find(thisPtr);
    if (calledIt != g_LootCalledWindows.end()) {
        DWORD callTime = calledIt->second;
        int itemCount = GetItemCount(container);

        if (itemCount <= 0) {
            g_DoneWindows[thisPtr] = now;
            g_LootCalledWindows.erase(calledIt);
            g_FirstSeenWindows.erase(thisPtr);
            return;
        }

        if (now - callTime > LOOT_GIVE_UP) {
            char msg[128];
            sprintf_s(msg, "[AL] Gave up on %p (%d items remain)", thisPtr, itemCount);
            LogWarning(msg);
            g_DoneWindows[thisPtr] = now;
            g_LootCalledWindows.erase(calledIt);
            g_FirstSeenWindows.erase(thisPtr);
            return;
        }

        if (now - callTime >= LOOT_RETRY_INTERVAL) {
            calledIt->second = now;
            g_TotalRetries++;
            SEH_InvokeLootAll(g_LootAllMethod, thisPtr);
        }
        return;
    }

    // ===== PHASE 3: Wait for items =====
    if (!HasContainerDetailsReceived(container)) {
        if (g_FirstSeenWindows.find(thisPtr) == g_FirstSeenWindows.end())
            g_FirstSeenWindows[thisPtr] = now;
        return;
    }

    int itemCount = GetItemCount(container);

    if (itemCount <= 0) {
        auto seenIt = g_FirstSeenWindows.find(thisPtr);
        if (seenIt == g_FirstSeenWindows.end()) {
            g_FirstSeenWindows[thisPtr] = now;
            return;
        }
        if (now - seenIt->second < ITEM_LOAD_GRACE) return;
        g_DoneWindows[thisPtr] = now;
        g_FirstSeenWindows.erase(seenIt);
        g_TotalSkippedEmpty++;
        return;
    }

    // ===== PHASE 4: Has items — delay then loot =====
    auto seenIt = g_FirstSeenWindows.find(thisPtr);
    if (seenIt == g_FirstSeenWindows.end()) {
        g_FirstSeenWindows[thisPtr] = now;
        return;
    }
    if (now - seenIt->second < LOOT_INITIAL_DELAY) return;

    if (now - g_LastLootAttempt < LOOT_COOLDOWN) return;
    g_LastLootAttempt = now;

    g_FirstSeenWindows.erase(thisPtr);

    char msg[128];
    sprintf_s(msg, "[AL] Looting %d items from %p", itemCount, thisPtr);
    LogInfo(msg);

    if (SEH_InvokeLootAll(g_LootAllMethod, thisPtr)) {
        g_TotalLooted++;
        g_LootCalledWindows[thisPtr] = now;
        LogSuccess("[AL] LootAll called!");
    }
    else {
        g_CWUpdateCrashCount++;
        char emsg[128]; sprintf_s(emsg, "[AL] LootAll failed! (#%d)", g_CWUpdateCrashCount);
        LogError(emsg);
    }
}

// ============================================
// HOOK INSTALL
// ============================================

static bool InstallMethodHook(void* methodInfo, void* hookFn, void** outOriginal) {
    if (!methodInfo || SEH_IsBadRead(methodInfo, sizeof(void*))) return false;
    void* originalPtr = nullptr;
    if (!SEH_ReadPtr(methodInfo, &originalPtr)) return false;
    if (!originalPtr || SEH_IsBadRead(originalPtr, 1)) return false;
    *outOriginal = originalPtr;

    DWORD oldProtect;
    if (!VirtualProtect(methodInfo, sizeof(void*), PAGE_READWRITE, &oldProtect)) return false;
    *(void**)(methodInfo) = hookFn;
    VirtualProtect(methodInfo, sizeof(void*), oldProtect, &oldProtect);
    return true;
}

// ============================================
// INIT
// ============================================

bool InitAutoLoot() {
    LogInfo("[AL] Initializing AutoLoot...");

    void* domain = Resolver::GetDomain();
    if (!domain) { LogError("[AL] No domain!"); return false; }

    size_t ac = 0;
    void** assemblies = Functions.domain_get_assemblies(domain, &ac);
    void* gameImage = nullptr;

    for (size_t i = 0; i < ac; i++) {
        void* image = Functions.assembly_get_image(assemblies[i]);
        if (!image) continue;
        const char* imgName = Functions.image_get_name(image);
        if (!imgName) continue;
        if (strcmp(imgName, "Game.dll") == 0) { gameImage = image; break; }
    }

    if (!gameImage) { LogError("[AL] Game.dll not found!"); return false; }

    void* klass;

    klass = Functions.class_from_name(gameImage, "", "ContainerWindow");
    if (klass) { g_ContainerWindowClass = Class(klass); LogSuccess("[AL] Found ContainerWindow"); }
    else { LogError("[AL] ContainerWindow not found!"); return false; }

    klass = Functions.class_from_name(gameImage, "", "Container");
    if (klass) { g_ContainerClass = Class(klass); LogSuccess("[AL] Found Container"); }
    else { LogError("[AL] Container not found!"); return false; }

    Method m;

    m = g_ContainerWindowClass.GetMethod("get_Container", 0);
    if (m.IsValid()) { g_GetContainerMethod = m.ptr; LogSuccess("[AL] Cached get_Container()"); }

    m = g_ContainerWindowClass.GetMethod("LootAll", 0);
    if (m.IsValid()) {
        g_LootAllMethod = m.ptr;
        LogSuccess("[AL] Found LootAll for runtime_invoke");
    }
    else { LogError("[AL] LootAll not found!"); return false; }

    m = g_ContainerWindowClass.GetMethod("Update", 0);
    if (m.IsValid()) {
        g_CWUpdateMethodInfo = m.ptr;
        if (InstallMethodHook(m.ptr, (void*)HookedContainerWindowUpdate, (void**)&g_OriginalCWUpdate)) {
            LogSuccess("[AL] Hooked CW.Update");
        }
        else { LogError("[AL] Failed to hook CW.Update!"); return false; }
    }
    else { LogError("[AL] CW.Update not found!"); return false; }

    LogSuccess("[AL] AutoLoot initialized!");
    return true;
}

bool IsAutoLootReady() {
    return g_ContainerWindowClass.IsValid() && g_LootAllMethod != nullptr && g_OriginalCWUpdate != nullptr;
}

void AutoLootUpdate() {
    // DO NOT PUT ANYTHING HERE
}

void ToggleAutoLoot() {
    if (!g_ContainerWindowClass.IsValid() || !g_OriginalCWUpdate) {
        LogWarning("[AL] Not initialized! Initializing now...");
        if (!InitAutoLoot()) {
            LogError("[AL] Failed to initialize!");
            return;
        }
    }

    g_AutoLootEnabled = !g_AutoLootEnabled;
    g_LootCalledWindows.clear();
    g_DoneWindows.clear();
    g_FirstSeenWindows.clear();
    g_CWUpdateCrashCount = 0;
    g_TotalLooted = 0;
    g_TotalSkippedEmpty = 0;
    g_TotalRetries = 0;
    g_LastDiagnosticLog = 0;
    g_LastLootAttempt = 0;
    g_LastMapTrim = 0;

    if (g_AutoLootEnabled)
        LogSuccess("[AL] AutoLoot ON!");
    else
        LogInfo("[AL] AutoLoot DISABLED");
}