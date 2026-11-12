#include <Windows.h>
#include <iostream>
#include <fstream>

constexpr bool SHOW_CONSOLE = true;
#include <vector>
#include <cstdint>
#include "memory/driver.h"
#include "render/render.h"

int main() {
    if (!SHOW_CONSOLE) {
        ShowWindow(GetConsoleWindow(), SW_HIDE);
    }

    if (SHOW_CONSOLE) std::cout << "Loading driver..." << std::endl;

    if (!driver.Init()) {
        if (SHOW_CONSOLE) {
            std::cout << "Failed to load driver" << std::endl;
            system("pause");
        }
        return 1;
    }
    if (SHOW_CONSOLE) std::cout << "Driver loaded" << std::endl << "Waiting for game..." << std::endl;

    while (!driver.FindProcess(L"WardogsClient-Win64-Shipping.exe")) {
        Sleep(100);
    }
    if (SHOW_CONSOLE) std::cout << "Game found (PID: " << driver.ProcessID << ")" << std::endl << "Fetching CR3..." << std::endl;

    uintptr_t cr3 = driver.FetchCR3();
    if (!cr3) {
        if (SHOW_CONSOLE) {
            std::cout << "Failed to get CR3" << std::endl;
            system("pause");
        }
        return 1;
    }
    if (SHOW_CONSOLE) std::cout << "CR3: 0x" << std::hex << cr3 << std::dec << std::endl;

    memory.BaseAddress = driver.GetBase();
    if (!memory.BaseAddress) {
        if (SHOW_CONSOLE) {
            std::cout << "Failed to get base address" << std::endl;
            system("pause");
        }
        return 1;
    }
    if (SHOW_CONSOLE) std::cout << "Base address: 0x" << std::hex << memory.BaseAddress << std::dec << std::endl << "Press insert to toggle menu" << std::endl;

    if (!overlay::Create()) {
        if (SHOW_CONSOLE) std::cout << "Failed to create overlay" << std::endl;
        return 1;
    }
    StartScanner(50);

    RenderLoop();

    StopScanner();

    return 0;
}
