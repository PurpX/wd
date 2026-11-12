#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>

class DRIVER_CLASS {
public:
    INT32 ProcessID = 0;
    uintptr_t CR3 = 0;

    bool Init() {
        return false;
    }

    uintptr_t FetchCR3() {
        return 0;
    }

    uintptr_t GetBase() {
        return 0;
    }

    void ReadPhysicalMemory(PVOID address, PVOID buffer, DWORD size) {
        (void)address;
        (void)buffer;
        (void)size;
    }

    void WritePhysicalMemory(PVOID address, PVOID buffer, DWORD size) {
        (void)address;
        (void)buffer;
        (void)size;
    }

    INT32 FindProcess(LPCTSTR name) {
        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(entry);
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

        if (snap == INVALID_HANDLE_VALUE)
            return 0;

        if (Process32First(snap, &entry)) {
            do {
                if (!lstrcmpi(entry.szExeFile, name)) {
                    ProcessID = entry.th32ProcessID;
                    CloseHandle(snap);
                    return ProcessID;
                }
            } while (Process32Next(snap, &entry));
        }
        CloseHandle(snap);
        return 0;
    }
};

inline DRIVER_CLASS driver;

class MEMORY_CLASS {
public:
    ULONGLONG BaseAddress = 0;

    template <typename T>
    T read(uint64_t addr) {
        T buf{};
        driver.ReadPhysicalMemory((PVOID)addr, &buf, sizeof(T));
        return buf;
    }

    template <typename T>
    void write(uint64_t addr, T val) {
        driver.WritePhysicalMemory((PVOID)addr, &val, sizeof(T));
    }
};

inline MEMORY_CLASS memory;
