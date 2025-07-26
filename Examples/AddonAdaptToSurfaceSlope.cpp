#include <windows.h>
#include "lua.hpp"
#include "MinHook.h"

// ----------------------------------------------------------------------------------
// Configuration: Addresses from IL2CPP dump and Lua script file name
// ----------------------------------------------------------------------------------
constexpr uintptr_t ADDR_CTOR = 10495216;                      // .ctor
constexpr uintptr_t ADDR_GET_FORCE_ADAPT_ROTATION = 9267152;   // get_forceAdaptRotationToSurfaceSlope
constexpr uintptr_t ADDR_SET_FORCE_ADAPT_ROTATION = 9267584;   // set_forceAdaptRotationToSurfaceSlope

const char* LUA_SCRIPT = "addon_slope.lua"; // Your Lua script filename

// ----------------------------------------------------------------------------------
// Globals
// ----------------------------------------------------------------------------------
uintptr_t g_addonInstance = 0; // Set by constructor hook
void* g_methodInfo = nullptr;  // For most games, nullptr is sufficient

// ----------------------------------------------------------------------------------
// Function pointer typedefs (from IL2CPP metadata)
// ----------------------------------------------------------------------------------
typedef void (__fastcall* tCtor)(void* __this, void* method);
typedef bool (__fastcall* tGetForceAdaptRotationToSurfaceSlope)(void* __this, void* method);
typedef void (__fastcall* tSetForceAdaptRotationToSurfaceSlope)(void* __this, bool value, void* method);

tCtor oCtor = nullptr;

// ----------------------------------------------------------------------------------
// Hooked constructor: store instance pointer
// ----------------------------------------------------------------------------------
void __fastcall hkCtor(void* __this, void* method) {
    g_addonInstance = reinterpret_cast<uintptr_t>(__this);
    oCtor(__this, method); // Call the original constructor
}

// ----------------------------------------------------------------------------------
// Lua bindings
// ----------------------------------------------------------------------------------
int lua_GetForceAdaptRotation(lua_State* L) {
    if (!g_addonInstance) {
        lua_pushboolean(L, false);
        return 1;
    }
    auto func = reinterpret_cast<tGetForceAdaptRotationToSurfaceSlope>(ADDR_GET_FORCE_ADAPT_ROTATION);
    bool value = func((void*)g_addonInstance, g_methodInfo);
    lua_pushboolean(L, value);
    return 1;
}

int lua_SetForceAdaptRotation(lua_State* L) {
    if (!g_addonInstance) return 0;
    bool enable = lua_toboolean(L, 1) != 0;
    auto func = reinterpret_cast<tSetForceAdaptRotationToSurfaceSlope>(ADDR_SET_FORCE_ADAPT_ROTATION);
    func((void*)g_addonInstance, enable, g_methodInfo);
    return 0;
}

void ExposeSlopeAPI(lua_State* L) {
    lua_register(L, "SetForceAdaptRotation", lua_SetForceAdaptRotation);
    lua_register(L, "GetForceAdaptRotation", lua_GetForceAdaptRotation);
}

// ----------------------------------------------------------------------------------
// Main mod thread: setup MinHook, Lua, run script
// ----------------------------------------------------------------------------------
DWORD WINAPI MainThread(LPVOID) {
    // Initialize MinHook
    if (MH_Initialize() != MH_OK) return 1;
    // Hook the constructor
    if (MH_CreateHook(reinterpret_cast<LPVOID>(ADDR_CTOR), &hkCtor, reinterpret_cast<LPVOID*>(&oCtor)) != MH_OK) return 1;
    if (MH_EnableHook(reinterpret_cast<LPVOID>(ADDR_CTOR)) != MH_OK) return 1;

    // LuaJIT state
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    ExposeSlopeAPI(L);

    // Try to run the Lua script
    int status = luaL_dofile(L, LUA_SCRIPT);
    if (status != LUA_OK) {
        MessageBoxA(NULL, lua_tostring(L, -1), "Lua Error", MB_OK | MB_ICONERROR);
    }

    // Simple event loop (extend as needed)
    while (true) {
        Sleep(100);
    }

    // Cleanup (never reached)
    lua_close(L);
    MH_DisableHook(reinterpret_cast<LPVOID>(ADDR_CTOR));
    MH_Uninitialize();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}