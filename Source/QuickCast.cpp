#include "QuickCast.h"
#include "Console.h"

// ============================================
// QUICK CAST STATE
// ============================================

bool g_QuickCastEnabled = false;
Class g_LocalPlayerInputClass;
Class g_CameraControllerClass;
Class g_SpellClass;
Class g_QuickBarClass;

static void* g_GetTargetingObjectMethod = nullptr;

static bool g_WasInTargetingMode = false;
static bool g_PendingClick = false;
static DWORD g_PendingClickTime = 0;
static int g_TargetingKey = 0;
static const DWORD CLICK_DELAY = 50;

bool InitQuickCast() {
    LogInfo("Initializing QuickCast...");

    void* domain = Resolver::GetDomain();
    if (!domain) { LogError("[QC] No domain!"); return false; }

    size_t ac = 0;
    void** assemblies = Functions.domain_get_assemblies(domain, &ac);
    void* gameImage = nullptr;

    for (size_t i = 0; i < ac; i++) {
        void* image = Functions.assembly_get_image(assemblies[i]);
        if (!image) continue;
        const char* imgName = Functions.image_get_name(image);
        if (imgName && strcmp(imgName, "Game.dll") == 0) {
            gameImage = image;
            break;
        }
    }

    if (!gameImage) { LogError("[QC] Game.dll not found!"); return false; }

    void* k;

    k = Functions.class_from_name(gameImage, "", "LocalPlayerInput");
    if (k) { g_LocalPlayerInputClass = Class(k); LogSuccess("[QC] Found LocalPlayerInput"); }
    else { LogError("[QC] LocalPlayerInput not found!"); return false; }

    k = Functions.class_from_name(gameImage, "", "CameraController");
    if (k) { g_CameraControllerClass = Class(k); LogSuccess("[QC] Found CameraController"); }
    else { LogWarning("[QC] CameraController not found"); }

    k = Functions.class_from_name(gameImage, "", "Spell");
    if (k) { g_SpellClass = Class(k); LogSuccess("[QC] Found Spell"); }
    else { LogWarning("[QC] Spell not found"); }

    Method m = g_LocalPlayerInputClass.GetMethod("get_TargetingObject", 0);
    if (m.IsValid()) { g_GetTargetingObjectMethod = m.ptr; LogSuccess("[QC] Cached get_TargetingObject"); }
    else { LogError("[QC] get_TargetingObject not found!"); return false; }

    LogSuccess("[QC] QuickCast initialized!");
    return true;
}

static void* GetLocalPlayerInput() {
    if (!g_LocalPlayerInputClass.IsValid()) return nullptr;

    Method findMethod = UnityObject.GetMethod("FindObjectsOfType", 1);
    if (!findMethod.IsValid()) return nullptr;

    void* typeObj = Functions.type_get_object(Functions.class_get_type(g_LocalPlayerInputClass.ptr));
    if (!typeObj) return nullptr;

    void* args[] = { typeObj };
    void* exc = nullptr;
    void* result = Functions.runtime_invoke(findMethod.ptr, nullptr, args, &exc);
    if (!result || exc) return nullptr;

    Il2CppArray* arr = (Il2CppArray*)result;
    if (IsBadReadPtr(arr, sizeof(Il2CppArray)) || arr->max_length == 0) return nullptr;
    return arr->vector[0];
}

static bool IsInTargetingModeDirect(void* inputInstance) {
    if (!inputInstance) return false;
    try {
        uintptr_t addr = (uintptr_t)inputInstance;
        if (IsBadReadPtr((void*)(addr + 0x88), sizeof(void*))) return false;
        void* targetObj = *(void**)(addr + 0x88);
        return (targetObj != nullptr);
    }
    catch (...) {
        return false;
    }
}

static void DoMouseClick() {
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    Sleep(10);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}

static bool IsSpellKey(int vk) {
    if (vk >= '1' && vk <= '9') return true;
    if (vk == '0') return true;
    if (vk == VK_OEM_MINUS || vk == VK_OEM_PLUS) return true;
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) return true;
    if (vk == 'Q' || vk == 'E' || vk == 'R' || vk == 'T') return true;
    if (vk == 'F' || vk == 'G' || vk == 'V' || vk == 'B') return true;
    if (vk == 'Z' || vk == 'X' || vk == 'C') return true;
    return false;
}

void QuickCastUpdate() {
    if (!g_QuickCastEnabled) return;

    static DWORD lastCheck = 0;
    DWORD now = (DWORD)GetTickCount64();

    if (now - lastCheck < 10) return;
    lastCheck = now;

    // Handle pending click
    if (g_PendingClick) {
        if (now - g_PendingClickTime >= CLICK_DELAY) {
            DoMouseClick();
            g_PendingClick = false;
            g_WasInTargetingMode = false;
            g_TargetingKey = 0;
            return;
        }
        return;
    }

    try {
        static void* cachedInput = nullptr;
        static DWORD lastCacheTime = 0;

        if (now - lastCacheTime > 5000 || !cachedInput) {
            cachedInput = GetLocalPlayerInput();
            lastCacheTime = now;
            if (!cachedInput) return;
        }

        if (IsBadReadPtr(cachedInput, sizeof(void*))) {
            cachedInput = nullptr;
            return;
        }

        uintptr_t addr = (uintptr_t)cachedInput;
        if (IsBadReadPtr((void*)(addr + 0x88), sizeof(void*))) {
            cachedInput = nullptr;
            return;
        }

        bool inTargeting = IsInTargetingModeDirect(cachedInput);

        if (inTargeting && !g_WasInTargetingMode) {
            g_WasInTargetingMode = true;
            g_TargetingKey = 0;
            for (int vk = 0; vk < 256; vk++) {
                if (!IsSpellKey(vk)) continue;
                if (GetAsyncKeyState(vk) & 0x8000) {
                    g_TargetingKey = vk;
                    break;
                }
            }
        }
        else if (inTargeting && g_WasInTargetingMode && !g_PendingClick) {
            for (int vk = 0; vk < 256; vk++) {
                if (!IsSpellKey(vk)) continue;
                if (GetAsyncKeyState(vk) & 1) {
                    g_PendingClick = true;
                    g_PendingClickTime = now;
                    return;
                }
            }
        }
        else if (!inTargeting && g_WasInTargetingMode) {
            g_WasInTargetingMode = false;
            g_TargetingKey = 0;
            g_PendingClick = false;
        }
    }
    catch (...) {
        g_WasInTargetingMode = false;
        g_PendingClick = false;
        g_TargetingKey = 0;
    }
}

void ToggleQuickCast() {
    if (!g_LocalPlayerInputClass.IsValid()) {
        LogWarning("[QC] Not initialized! Initializing now...");
        if (!InitQuickCast()) {
            LogError("[QC] Failed to initialize!");
            return;
        }
    }

    g_QuickCastEnabled = !g_QuickCastEnabled;

    if (g_QuickCastEnabled) {
        LogSuccess("[QC] QuickCast ENABLED - press spell key again to place!");
    }
    else {
        LogInfo("[QC] QuickCast DISABLED");
        g_WasInTargetingMode = false;
        g_PendingClick = false;
        g_TargetingKey = 0;
    }
}