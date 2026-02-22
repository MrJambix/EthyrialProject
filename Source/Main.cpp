#include "Globals.h"
#include "Console.h"
#include "Classes.h"
#include "Config.h"
#include "Player.h"
#include "Overlay.h"
#include "Stacks.h"
#include "Raid.h"
#include "Settings.h"
#include "Dump.h"
#include "QuickCast.h"
#include "AutoLoot.h"
#include "MemoryGuard.h"
#include <iostream>
#include <conio.h>

// ============================================
// GLOBAL DEFINITIONS
// ============================================

Class g_StatusEffectClass;
Class g_LivingEntityClass;
Class g_LivingEntityModelClass;
Class g_PlayerScriptClass;

void* g_PlayerEntity = nullptr;
void* g_PlayerModel = nullptr;
std::string g_PlayerName = "";

std::vector<StatusEffectInfo> g_CurrentEffects;
std::map<std::string, TrackedStackEffect> g_TrackedStacks;
std::vector<EntityEntry> g_EntityList;
std::vector<std::string> g_RaidTrackNames;
std::vector<RaidMemberData> g_RaidMembers;

std::mutex g_EffectsMutex;
std::mutex g_StacksMutex;
std::mutex g_RaidMutex;
std::mutex g_ConfigMutex;

FilterMode g_FilterMode = FILTER_SHOW_ALL;
std::map<std::string, BuffEntry> g_BuffConfig;
std::vector<std::string> g_ConfigDisplayOrder;

bool g_OverlayActive = true;
bool g_StacksOverlayActive = true;
bool g_RaidOverlayActive = true;
bool g_TrackingActive = false;
bool g_WaitingForSelection = false;
bool g_WaitingForConfig = false;
bool g_WaitingForRaidConfig = false;

// Single window, but 3 independent panel positions
HWND g_OverlayHwnd = nullptr;
ULONG_PTR g_GdiplusToken = 0;

int g_OverlayX = 100, g_OverlayY = 100;   // Buff panel screen pos
int g_StacksX = 100, g_StacksY = 400;     // Stacks panel screen pos
int g_RaidX = 400, g_RaidY = 100;         // Raid panel screen pos

const char* CONFIG_FILE = "ethyrial_buffs.cfg";
const char* RAID_CONFIG_FILE = "ethyrial_raid.cfg";

// ============================================
// SHARED HELPERS
// ============================================

std::string ReadIL2CppString(void* strPtr) {
    if (!strPtr || IsBadReadPtr(strPtr, sizeof(void*))) return "";
    try {
        Il2CppString* str = (Il2CppString*)strPtr;
        if (IsBadReadPtr(&str->length, sizeof(int))) return "";
        if (str->length <= 0 || str->length > 1024) return "";
        if (IsBadReadPtr(str->chars, str->length * sizeof(wchar_t))) return "";
        std::wstring ws(str->chars, str->length);
        std::string result;
        result.reserve(ws.size());
        for (wchar_t c : ws) { if (c < 128) result += (char)c; else result += '?'; }
        return result;
    }
    catch (...) { return ""; }
}

std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return L"";
    int sz = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    if (sz <= 0) return L"";
    std::wstring ws(sz, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &ws[0], sz);
    return ws;
}

std::string ToLower(const std::string& s) {
    std::string r = s;
    for (char& c : r) if (c >= 'A' && c <= 'Z') c += 32;
    return r;
}

// ============================================
// WINDOW CREATION — SINGLE UNIFIED OVERLAY
// ============================================

static bool CreateOverlayWindow() {
    LogInfo("Registering unified overlay class...");

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = UnifiedWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"EthyrialOverlay";

    if (!RegisterClassEx(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        LogError("Overlay class failed!");
        return false;
    }

    int winX, winY;
    CalcWindowOrigin(winX, winY);

    g_OverlayHwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        L"EthyrialOverlay", L"Ethyrial Overlay",
        WS_POPUP,
        winX, winY, 1, 1,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    if (!g_OverlayHwnd) {
        LogError("Overlay window failed!");
        return false;
    }

    SetLayeredWindowAttributes(g_OverlayHwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(g_OverlayHwnd, SW_SHOW);

    LogSuccess("Unified overlay window created!");
    return true;
}

// ============================================
// OVERLAY THREAD
// ============================================

DWORD WINAPI OverlayThread(LPVOID) {
    LogInfo("[OVERLAY] Thread starting...");
    try {
        GdiplusStartupInput gsi;
        ULONG_PTR token = 0;
        if (GdiplusStartup(&token, &gsi, nullptr) != Gdiplus::Ok) { LogError("GDI+ failed!"); return 1; }
        g_GdiplusToken = token;
        LogSuccess("GDI+ OK!");

        if (!CreateOverlayWindow()) { LogError("Window creation failed!"); return 1; }

        Sleep(1000);
        LogSuccess("[OVERLAY] Entering message loop!");

        MSG msg;
        DWORD lastRepaint = 0;
        const DWORD REPAINT_INTERVAL = 100;

        while (true) {
            try {
                while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                CreateSettingsIfRequested();

                DWORD now = GetTickCount();

                if (IsMemoryEmergency()) {
                    Sleep(100);
                    continue;
                }

                if (now - lastRepaint >= REPAINT_INTERVAL) {
                    lastRepaint = now;

                    if (g_OverlayHwnd && IsWindow(g_OverlayHwnd)) {
                        int totalW = CalcTotalOverlayWidth();
                        int totalH = CalcTotalOverlayHeight();

                        if (totalW > 0 && totalH > 0) {
                            int winX, winY;
                            CalcWindowOrigin(winX, winY);

                            // Reposition + resize window to cover all panels
                            SetWindowPos(g_OverlayHwnd, HWND_TOPMOST,
                                winX, winY,
                                totalW + 4, totalH + 4,
                                SWP_NOZORDER);

                            if (!IsWindowVisible(g_OverlayHwnd))
                                ShowWindow(g_OverlayHwnd, SW_SHOWNOACTIVATE);

                            InvalidateRect(g_OverlayHwnd, nullptr, FALSE);
                        }
                        else {
                            if (IsWindowVisible(g_OverlayHwnd))
                                ShowWindow(g_OverlayHwnd, SW_HIDE);
                        }
                    }

                    if (g_SettingsHwnd && IsWindow(g_SettingsHwnd) && IsWindowVisible(g_SettingsHwnd))
                        InvalidateRect(g_SettingsHwnd, nullptr, FALSE);

                    static DWORD lastGdiCheck = 0;
                    if (now - lastGdiCheck > 10000) {
                        lastGdiCheck = now;
                        DWORD gdiCount = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
                        if (gdiCount > 5000) {
                            char gm[128];
                            sprintf_s(gm, "[OVERLAY] GDI objects = %lu!", gdiCount);
                            LogWarning(gm);
                        }
                    }
                }
            }
            catch (...) { LogError("[OVERLAY] Exception!"); }
            Sleep(16);
        }
    }
    catch (...) { LogError("[OVERLAY] FATAL!"); }
    return 0;
}

// ============================================
// DATA THREAD
// ============================================

DWORD WINAPI DataThread(LPVOID) {
    LogInfo("[DATA] Thread starting...");
    DWORD lastRaidUpdate = 0;
    DWORD lastDataUpdate = 0;
    const DWORD RAID_INTERVAL = 2000;
    const DWORD DATA_INTERVAL = 500;

    while (true) {
        try {
            DWORD now = GetTickCount();

            UpdateMemoryGuard();

            if (!WasQuickCastPausedByMemory()) {
                QuickCastUpdate();
            }

            if (IsMemoryDataSafe() && now - lastDataUpdate >= DATA_INTERVAL) {
                lastDataUpdate = now;
                CheckMemoryPeriodic();

                if (g_TrackingActive && g_PlayerEntity) {
                    if (!IsBadReadPtr(g_PlayerEntity, sizeof(void*))) {
                        std::string name = GetEntityName(g_PlayerEntity);
                        if (name.empty()) {
                            g_PlayerEntity = nullptr; g_TrackingActive = false;
                            LogWarning("[DATA] Player pointer stale.");
                        }
                        else {
                            auto fx = ReadEffectsFromEntity(g_PlayerEntity);
                            SortEffects(fx);
                            UpdateTrackedStacks(fx);
                            { std::lock_guard<std::mutex> lock(g_EffectsMutex); g_CurrentEffects = fx; }

                            if (now - lastRaidUpdate >= RAID_INTERVAL) {
                                lastRaidUpdate = now;
                                bool hasRaid = false;
                                { std::lock_guard<std::mutex> lock(g_RaidMutex); hasRaid = !g_RaidTrackNames.empty(); }
                                if (hasRaid) { try { UpdateRaidData(); } catch (...) { LogError("[DATA] Raid exception!"); } }
                            }
                        }
                    }
                    else {
                        g_PlayerEntity = nullptr; g_TrackingActive = false;
                        LogError("[DATA] Bad pointer!");
                    }
                }

                {
                    std::lock_guard<std::mutex> lock(g_StacksMutex);
                    auto it = g_TrackedStacks.begin();
                    while (it != g_TrackedStacks.end()) {
                        if (!it->second.isActive && it->second.lastDropTime > 0 && now - it->second.lastDropTime > (DWORD)(STACKS_FADE_TIME + 5000))
                            it = g_TrackedStacks.erase(it);
                        else ++it;
                    }
                }
            }
        }
        catch (...) { LogError("[DATA] Exception!"); }
        Sleep(16);
    }
    return 0;
}

// ============================================
// INPUT THREAD
// ============================================

DWORD WINAPI InputThread(LPVOID) {
    LogInfo("[INPUT] Thread starting...");
    while (true) {
        try {
            if (GetAsyncKeyState(VK_F1) & 1) {
                LogInfo("F1 - finding player...");
                if (FindPlayerEntity()) { g_TrackingActive = true; LogSuccess(("Found: " + g_PlayerName).c_str()); }
                else { LogWarning("Auto-detect failed."); RequestEntityPicker(); }
            }
            if (GetAsyncKeyState(VK_F2) & 1) {
                if (g_PlayerEntity) { g_TrackingActive = !g_TrackingActive; LogInfo(g_TrackingActive ? "Tracking ON" : "Tracking OFF"); }
                else LogError("No player! Press F1 first.");
            }
            if (GetAsyncKeyState(VK_F3) & 1) {
                g_OverlayActive = !g_OverlayActive;
                LogInfo(g_OverlayActive ? "Buff panel ON" : "Buff panel OFF");
            }
            if (GetAsyncKeyState(VK_F4) & 1) {
                g_StacksOverlayActive = !g_StacksOverlayActive;
                LogInfo(g_StacksOverlayActive ? "Stacks panel ON" : "Stacks panel OFF");
            }
            if (GetAsyncKeyState(VK_F5) & 1) { LogInfo("F5 - buff config..."); ShowConfigMenu(); }
            if (GetAsyncKeyState(VK_F6) & 1) { LogInfo("F6 - listing entities..."); ListAllLivingEntities(); }
            if (GetAsyncKeyState(VK_F7) & 1) {
                g_TrackingActive = false; g_PlayerEntity = nullptr; g_PlayerModel = nullptr; g_PlayerName = "";
                { std::lock_guard<std::mutex> lock(g_EffectsMutex); g_CurrentEffects.clear(); }
                LogWarning("Player reset!");
            }
            if (GetAsyncKeyState(VK_F8) & 1) { { std::lock_guard<std::mutex> lock(g_StacksMutex); g_TrackedStacks.clear(); } LogSuccess("Stacks cleared!"); }
            if (GetAsyncKeyState(VK_F9) & 1) { LogInfo("F9 - raid config..."); ShowRaidConfigMenu(); }
            if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState('R') & 1)) {
                g_RaidOverlayActive = !g_RaidOverlayActive;
                LogInfo(g_RaidOverlayActive ? "Raid panel ON" : "Raid panel OFF");
            }
            if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState('M') & 1)) {
                LogInfo("CTRL+M - forcing memory cleanup...");
                ForceMemoryCleanup();
                PrintMemoryUsage();
            }
            if (GetAsyncKeyState(VK_F11) & 1) {
                LogInfo("F11 - settings...");
                if (IsSettingsOpen()) CloseSettingsWindow(); else RequestOpenSettings();
            }
            if (GetAsyncKeyState(VK_F12) & 1) {
                LogInfo("F12 - opening class dumper...");
                DumpGameClasses();
            }
            if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState('Q') & 1)) {
                ToggleQuickCast();
            }
            if (GetAsyncKeyState(VK_END) & 1) {
                LogHeader("SAVING AND UNLOADING");
                SaveConfig(); SaveRaidConfig(); LogSuccess("Configs saved!");
                LogWarning("Unloading in 2 seconds..."); Sleep(2000);
                if (g_OverlayHwnd && IsWindow(g_OverlayHwnd)) DestroyWindow(g_OverlayHwnd);
                if (g_SettingsHwnd && IsWindow(g_SettingsHwnd)) DestroyWindow(g_SettingsHwnd);
                CloseDumpConsole();
                if (g_GdiplusToken) GdiplusShutdown(g_GdiplusToken);
                FreeConsole();
                FreeLibraryAndExitThread(GetModuleHandle(nullptr), 0);
                return 0;
            }

            if (g_WaitingForSelection && _kbhit()) {
                std::string line; std::getline(std::cin, line);
                if (!line.empty()) { try { SelectEntity(std::stoi(line)); } catch (...) { LogError("Invalid!"); g_WaitingForSelection = false; } }
            }
            if (g_WaitingForConfig && _kbhit()) {
                std::string line; std::getline(std::cin, line);
                HandleConfigInput(line);
            }
            if (g_WaitingForRaidConfig && _kbhit()) {
                std::string line; std::getline(std::cin, line);
                HandleRaidConfigInput(line);
            }
        }
        catch (...) { LogError("[INPUT] Exception!"); }
        Sleep(50);
    }
    return 0;
}

// ============================================
// INITIALIZATION
// ============================================

static void Initialize() {
    InitConsole();
    InstallCrashHandlers();
    LogHeader("INITIALIZING");

    LogInfo("Step 1: IL2CPP...");
    try {
        if (!Resolver::Initialize()) { LogError("IL2CPP failed!"); return; }
    }
    catch (...) { LogError("IL2CPP exception!"); return; }
    LogSuccess("IL2CPP done!");

    LogInfo("Step 2: Game classes...");
    if (!FindAllClasses()) LogWarning("Some classes not found!");
    LogSuccess("Classes done!");

    LogInfo("Step 2b: QuickCast...");
    if (InitQuickCast()) LogSuccess("QuickCast ready!");
    else LogWarning("QuickCast init failed (can retry with CTRL+Q)");

    LogInfo("Step 2c: AutoLoot...");
    if (InitAutoLoot()) LogSuccess("AutoLoot ready!");
    else LogWarning("AutoLoot init failed (can retry via Settings > Misc)");

    LogInfo("Step 3: Loading configs...");
    LoadConfig(); LoadRaidConfig();
    LogSuccess("Configs done!");

    LogInfo("Step 4: Starting threads...");
    HANDLE h;
    h = CreateThread(nullptr, 0, OverlayThread, nullptr, 0, nullptr); if (h) { LogSuccess("Overlay thread OK"); CloseHandle(h); }
    h = CreateThread(nullptr, 0, DataThread, nullptr, 0, nullptr); if (h) { LogSuccess("Data thread OK"); CloseHandle(h); }
    h = CreateThread(nullptr, 0, InputThread, nullptr, 0, nullptr); if (h) { LogSuccess("Input thread OK"); CloseHandle(h); }

    LogHeader("READY");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
    std::cout << "  F1=Find  F2=Track  F3=Buffs  F4=Stacks  F5=Config\n";
    std::cout << "  F6=List  F7=Reset  F8=Clear  F9=Raid\n";
    std::cout << "  F11=Settings  F12=Dumper  CTRL+Q=QuickCast  END=Unload\n";
    std::cout << "  CTRL+R=Raid Panel  CTRL+M=MemClean\n";
    std::cout << "  Drag each panel header independently!\n";
    std::cout << "  MemGuard: Warn=" << MEM_WARNING_MB << "MB Crit=" << MEM_CRITICAL_MB << "MB Emrg=" << MEM_EMERGENCY_MB << "MB\n\n";
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    PrintMemoryUsage();
}

// ============================================
// DLL ENTRY
// ============================================

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        HANDLE h = CreateThread(nullptr, 0, [](LPVOID) -> DWORD { Initialize(); return 0; }, nullptr, 0, nullptr);
        if (h) CloseHandle(h);
    }
    return TRUE;
}