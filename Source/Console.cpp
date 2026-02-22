#include "Console.h"
#include "Globals.h"
#include <iostream>
#include <cstdio>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

static HANDLE g_ConsoleHandle = nullptr;
static bool g_DebugMode = false;  // OFF by default — enable with a toggle if needed

static SIZE_T g_PeakMemoryMB = 0;
static SIZE_T g_LastMemoryMB = 0;
static DWORD g_LastMemCheck = 0;
static const DWORD MEM_CHECK_INTERVAL = 30000;  // 30 seconds instead of 5
static const SIZE_T MEM_WARNING_MB = 200;
static const SIZE_T MEM_CRITICAL_MB = 500;
static SIZE_T g_BaselineMemoryMB = 0;

static void SetColor(int color) {
    if (g_ConsoleHandle) SetConsoleTextAttribute(g_ConsoleHandle, color);
}

void InitConsole() {
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONIN$", "r", stdin);
    g_ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTitle(L"Ethyrial Buff Tracker - Debug Console");

    SetColor(14);
    std::cout << "\n========================================\n";
    std::cout << "  ETHYRIAL BUFF TRACKER\n";
    std::cout << "  Buffs + Stacks + Raid Tracker\n";
    std::cout << "========================================\n\n";
    SetColor(7);

    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        g_BaselineMemoryMB = pmc.WorkingSetSize / (1024 * 1024);
        g_PeakMemoryMB = g_BaselineMemoryMB;
        g_LastMemoryMB = g_BaselineMemoryMB;
    }
}

void Log(const char* msg) {
    LogInfo(msg);
}

void Log(const char* msg, const char* arg) {
    char buf[512];
    sprintf_s(buf, "%s %s", msg, arg);
    LogInfo(buf);
}

void Log(const char* msg, int arg) {
    char buf[512];
    sprintf_s(buf, "%s %d", msg, arg);
    LogInfo(buf);
}

void LogInfo(const char* msg) {
    SetColor(11);
    std::cout << "[INFO] " << msg << "\n";
    SetColor(7);
}

void LogSuccess(const char* msg) {
    SetColor(10);
    std::cout << "[OK] " << msg << "\n";
    SetColor(7);
}

void LogWarning(const char* msg) {
    SetColor(14);
    std::cout << "[WARN] " << msg << "\n";
    SetColor(7);
}

void LogError(const char* msg) {
    SetColor(12);
    std::cout << "[ERROR] " << msg << "\n";
    SetColor(7);
}

void LogHeader(const char* msg) {
    SetColor(13);
    std::cout << "\n========================================\n";
    std::cout << "  " << msg << "\n";
    std::cout << "========================================\n\n";
    SetColor(7);
}

void LogDebug(const char* msg) {
    if (!g_DebugMode) return;
    SetColor(8);
    std::cout << "[DEBUG] " << msg << "\n";
    SetColor(7);
}

void PrintMemoryUsage() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (!GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return;  // Silently fail — no need to spam error for a diagnostic function
    }

    SIZE_T workingMB = pmc.WorkingSetSize / (1024 * 1024);
    SIZE_T privateMB = pmc.PrivateUsage / (1024 * 1024);
    SIZE_T peakMB = pmc.PeakWorkingSetSize / (1024 * 1024);
    if (workingMB > g_PeakMemoryMB) g_PeakMemoryMB = workingMB;

    SIZE_T delta = workingMB > g_BaselineMemoryMB ? workingMB - g_BaselineMemoryMB : 0;

    if (delta >= MEM_CRITICAL_MB) {
        char msg[256];
        sprintf_s(msg, "MEMORY CRITICAL: %lluMB (delta +%lluMB, peak %lluMB)",
            (unsigned long long)workingMB, (unsigned long long)delta, (unsigned long long)peakMB);
        LogError(msg);

        DWORD gdiCount = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
        if (gdiCount > 5000) {
            char gdiMsg[128];
            sprintf_s(gdiMsg, "  GDI objects: %lu", gdiCount);
            LogError(gdiMsg);
        }
    }
    else if (delta >= MEM_WARNING_MB) {
        char msg[256];
        sprintf_s(msg, "MEMORY WARNING: %lluMB (delta +%lluMB)",
            (unsigned long long)workingMB, (unsigned long long)delta);
        LogWarning(msg);
    }
    // Normal memory — stay silent

    g_LastMemoryMB = workingMB;
}

void CheckMemoryPeriodic() {
    DWORD now = GetTickCount();
    if (now - g_LastMemCheck < MEM_CHECK_INTERVAL) return;
    g_LastMemCheck = now;
    PrintMemoryUsage();
}

// ============================================
// CRASH DIAGNOSTICS — only prints on actual crash
// ============================================

static void PrintCrashDiagnostics() {
    SetColor(12);
    std::cout << "\n!!! CRASH DIAGNOSTICS !!!\n";

    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        SIZE_T workingMB = pmc.WorkingSetSize / (1024 * 1024);
        SIZE_T peakMB = pmc.PeakWorkingSetSize / (1024 * 1024);
        SIZE_T delta = workingMB > g_BaselineMemoryMB ? workingMB - g_BaselineMemoryMB : 0;
        char msg[256];
        sprintf_s(msg, "  Memory: %lluMB (peak %lluMB, delta +%lluMB)",
            (unsigned long long)workingMB, (unsigned long long)peakMB, (unsigned long long)delta);
        std::cout << msg << "\n";
    }

    DWORD gdiCount = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    DWORD userCount = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
    char handleMsg[128];
    sprintf_s(handleMsg, "  GDI: %lu | USER: %lu", gdiCount, userCount);
    std::cout << handleMsg << "\n";
    if (gdiCount > 8000) std::cout << "  >>> GDI LIMIT APPROACHING!\n";

    DWORD handleCount = 0;
    GetProcessHandleCount(GetCurrentProcess(), &handleCount);
    sprintf_s(handleMsg, "  Handles: %lu", handleCount);
    std::cout << handleMsg << "\n";

    char stateMsg[256];
    sprintf_s(stateMsg, "  Player: '%s' | Tracking: %s | Entity: %p",
        g_PlayerName.c_str(), g_TrackingActive ? "ON" : "OFF", g_PlayerEntity);
    std::cout << stateMsg << "\n";

    std::cout << "\n";
    SetColor(7);
}

static const char* ExceptionCodeToString(DWORD code) {
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION: return "ACCESS_VIOLATION";
    case EXCEPTION_STACK_OVERFLOW: return "STACK_OVERFLOW";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "FLOAT_DIVIDE_BY_ZERO";
    case EXCEPTION_INT_DIVIDE_BY_ZERO: return "INT_DIVIDE_BY_ZERO";
    case EXCEPTION_ILLEGAL_INSTRUCTION: return "ILLEGAL_INSTRUCTION";
    case EXCEPTION_IN_PAGE_ERROR: return "IN_PAGE_ERROR";
    case EXCEPTION_INVALID_HANDLE: return "INVALID_HANDLE";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "NONCONTINUABLE";
    case 0xC0000374: return "HEAP_CORRUPTION";
    case 0xC0000409: return "STACK_BUFFER_OVERRUN";
    default: return "UNKNOWN";
    }
}

static LONG WINAPI CrashHandler(EXCEPTION_POINTERS* ep) {
    SetColor(12);
    std::cout << "\n========================================\n";
    std::cout << "  !!! CRASH DETECTED !!!\n";
    std::cout << "========================================\n\n";

    if (ep && ep->ExceptionRecord) {
        DWORD code = ep->ExceptionRecord->ExceptionCode;
        char msg[512];
        sprintf_s(msg, "  Exception: 0x%08X (%s)", code, ExceptionCodeToString(code));
        std::cout << msg << "\n";
        sprintf_s(msg, "  Address: 0x%p", ep->ExceptionRecord->ExceptionAddress);
        std::cout << msg << "\n";

        if (code == EXCEPTION_ACCESS_VIOLATION && ep->ExceptionRecord->NumberParameters >= 2) {
            const char* op = ep->ExceptionRecord->ExceptionInformation[0] == 0 ? "reading" : "writing";
            sprintf_s(msg, "  Fault: %s 0x%p", op, (void*)ep->ExceptionRecord->ExceptionInformation[1]);
            std::cout << msg << "\n";
            if (ep->ExceptionRecord->ExceptionInformation[1] < 0x10000)
                std::cout << "  >>> NULL POINTER DEREFERENCE\n";
        }

        HMODULE hMod = nullptr;
        char modName[MAX_PATH] = "unknown";
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)ep->ExceptionRecord->ExceptionAddress, &hMod))
            GetModuleFileNameA(hMod, modName, MAX_PATH);
        sprintf_s(msg, "  Module: %s", modName);
        std::cout << msg << "\n";

        if (ep->ContextRecord) {
            std::cout << "\n  REGISTERS:\n";
#ifdef _WIN64
            sprintf_s(msg, "  RAX=%016llX RBX=%016llX RCX=%016llX", ep->ContextRecord->Rax, ep->ContextRecord->Rbx, ep->ContextRecord->Rcx);
            std::cout << msg << "\n";
            sprintf_s(msg, "  RDX=%016llX RSI=%016llX RDI=%016llX", ep->ContextRecord->Rdx, ep->ContextRecord->Rsi, ep->ContextRecord->Rdi);
            std::cout << msg << "\n";
            sprintf_s(msg, "  RSP=%016llX RBP=%016llX RIP=%016llX", ep->ContextRecord->Rsp, ep->ContextRecord->Rbp, ep->ContextRecord->Rip);
            std::cout << msg << "\n";
#else
            sprintf_s(msg, "  EAX=%08X EBX=%08X ECX=%08X EDX=%08X", ep->ContextRecord->Eax, ep->ContextRecord->Ebx, ep->ContextRecord->Ecx, ep->ContextRecord->Edx);
            std::cout << msg << "\n";
#endif
        }
    }

    PrintCrashDiagnostics();
    SetColor(14);
    std::cout << "Press Enter to close...\n";
    SetColor(7);
    std::cin.get();
    return EXCEPTION_EXECUTE_HANDLER;
}

static void PureCallHandler() { LogError("PURE VIRTUAL CALL!"); PrintCrashDiagnostics(); }
static void InvalidParamHandler(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t) { LogError("INVALID PARAMETER!"); PrintCrashDiagnostics(); }
static void AbortHandler(int) { LogError("ABORT!"); PrintCrashDiagnostics(); }

void InstallCrashHandlers() {
    SetUnhandledExceptionFilter(CrashHandler);
    _set_purecall_handler(PureCallHandler);
    _set_invalid_parameter_handler(InvalidParamHandler);
    signal(SIGABRT, AbortHandler);
    LogSuccess("Crash handlers installed");
}