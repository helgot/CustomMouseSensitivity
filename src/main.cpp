#include <Windows.h>
#include "mod.h"
#include <print>

DWORD WINAPI InitializeModThread(LPVOID hModule)
{
    Mod::GetInstance().Start();
    std::string name = "test";
    std::println("Hello, {}", name);
    
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpvReversed) {
    switch (fdwReason) 
    {
        case DLL_PROCESS_ATTACH:
        {
            // We don't want to receive thread attach/detach notifications,
            // which can help avoid deadlocks.
            DisableThreadLibraryCalls(hModule);
            // Create a new thread to initialize our hooks.
            // This avoids blocking the game's main thread inside DllMain.
            HANDLE hThread = CreateThread(nullptr, 0, InitializeModThread, hModule, 0, nullptr);
            if (hThread)
            {
                CloseHandle(hThread); // We don't need to manage the thread, so close the handle.
            }
            break;
        }
        
        case DLL_PROCESS_DETACH:
        Mod::GetInstance().Shutdown();
        break;
    }
    return TRUE;
}