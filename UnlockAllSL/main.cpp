#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <string>

DWORD GetProcessIdByName(const std::string& processName)
{
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    if (Process32First(snapshot, &entry)) {
        do {
            if (processName == entry.szExeFile) {
                CloseHandle(snapshot);
                return entry.th32ProcessID;
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return 0;
}

uintptr_t GetBaseAddress(DWORD processId)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (!hProcess)
        return 0;

    HMODULE hMods[1024];
    DWORD cbNeeded;
    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        uintptr_t baseAddress = (uintptr_t)hMods[0];
        CloseHandle(hProcess);
        return baseAddress;
    }

    CloseHandle(hProcess);
    return 0;
}

bool PatchMemory(HANDLE hProcess, uintptr_t address, const uint8_t* bytes, size_t size)
{
    return WriteProcessMemory(hProcess, (LPVOID)address, bytes, size, nullptr);
}

int main()
{
    std::cout << "Waiting for user input... \nF1 = Unlock All \nF2 = Reset \nF3 = Exit\n";

    DWORD processId = GetProcessIdByName("RainbowSix.exe");
    if (processId == 0) {
        std::cout << "Process not found.\n";
        return 1;
    }

    uintptr_t baseAddress = GetBaseAddress(processId);
    if (baseAddress == 0) {
        std::cout << "Base address not found.\n";
        return 1;
    }

    uintptr_t patchAddress = baseAddress + 0x3843080;

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        std::cout << "Failed to open process.\n";
        return 1;
    }

    const uint8_t unlockBytes[4] = { 0x41, 0xB4, 0x00, 0x90 }; // Unlock All
    const uint8_t resetBytes[4] = { 0x41, 0xB4, 0x01, 0x90 }; // Reset (set r12l = 1)

    while (true) {
        if (GetAsyncKeyState(VK_F1) & 1) {
            if (PatchMemory(hProcess, patchAddress, unlockBytes, sizeof(unlockBytes)))
                std::cout << "[+] Unlock All applied.\n";
            else
                std::cout << "[-] Failed to apply Unlock All.\n";
        }

        if (GetAsyncKeyState(VK_F2) & 1) {
            if (PatchMemory(hProcess, patchAddress, resetBytes, sizeof(resetBytes)))
                std::cout << "[+] Reset applied.\n";
            else
                std::cout << "[-] Failed to apply Reset.\n";
        }

        if (GetAsyncKeyState(VK_F3) & 1) {
            std::cout << "Exiting...\n";
            break;
        }

        Sleep(100);
    }

    CloseHandle(hProcess);
    return 0;
}
