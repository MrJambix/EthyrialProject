#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <iostream>

DWORD FindProcessId(const std::wstring& processName) {
    PROCESSENTRY32 processInfo;
    processInfo.dwSize = sizeof(processInfo);

    HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (processesSnapshot == INVALID_HANDLE_VALUE)
        return 0;

    Process32First(processesSnapshot, &processInfo);
    if (!processName.compare(processInfo.szExeFile)) {
        CloseHandle(processesSnapshot);
        return processInfo.th32ProcessID;
    }

    while (Process32Next(processesSnapshot, &processInfo)) {
        if (!processName.compare(processInfo.szExeFile)) {
            CloseHandle(processesSnapshot);
            return processInfo.th32ProcessID;
        }
    }
    CloseHandle(processesSnapshot);
    return 0;
}

bool InjectDLL(DWORD processID, const std::string& dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (!hProcess)
        return false;

    LPVOID pDllPath = VirtualAllocEx(hProcess, 0, dllPath.length() + 1, MEM_COMMIT, PAGE_READWRITE);
    WriteProcessMemory(hProcess, pDllPath, (LPVOID)dllPath.c_str(), dllPath.length() + 1, 0);

    HANDLE hThread = CreateRemoteThread(hProcess, 0, 0,
        (LPTHREAD_START_ROUTINE)LoadLibraryA, pDllPath, 0, 0);
    if (!hThread)
        return false;

    WaitForSingleObject(hThread, INFINITE);

    VirtualFreeEx(hProcess, pDllPath, dllPath.length() + 1, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);
    return true;
}

int main() {
    const std::wstring processName = L"Ethyrial.exe";
    std::string dllPath;

    std::wcout << L"Auto-detecting process: " << processName << std::endl;

    std::cout << "Enter full path to your DLL: ";
    std::getline(std::cin, dllPath);

    DWORD pid = FindProcessId(processName);
    if (!pid) {
        std::wcout << L"Could not find process: " << processName << std::endl;
        return 1;
    }

    if (InjectDLL(pid, dllPath)) {
        std::cout << "DLL injected successfully!" << std::endl;
    }
    else {
        std::cout << "DLL injection failed!" << std::endl;
    }

    return 0;
}