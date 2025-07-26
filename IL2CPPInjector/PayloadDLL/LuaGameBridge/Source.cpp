#include "pch.h"
#include <Windows.h>
#include <string>
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include "game/il2cpp.h"

std::string GetModuleDirectory() {
    char path[MAX_PATH];
    HMODULE hModule = nullptr;

    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&GetModuleDirectory, &hModule);
    GetModuleFileNameA(hModule, path, MAX_PATH);

    std::string fullPath(path);
    size_t pos = fullPath.find_last_of("\\/");
    if (pos != std::string::npos) {
        return fullPath.substr(0, pos);
    }
    return fullPath;
}

int Lua_SomeGameFunction(lua_State* L) {
    lua_pushinteger(L, 123);
    return 1;
}

DWORD WINAPI MainThread(LPVOID) {
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    lua_register(L, "SomeGameFunction", Lua_SomeGameFunction);

    std::string dllDir = GetModuleDirectory();
    std::string luaScriptPath = dllDir + "\\..\\lua_scripts\\myscript.lua";


    luaL_dofile(L, luaScriptPath.c_str());

    while (true) Sleep(100);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, MainThread, NULL, 0, NULL);
        break;
    }
    return TRUE;
}