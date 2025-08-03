#include <Windows.h>
#include "ModManager.h"
#include "Logger.h"

// The main function for our mod's thread.
// This is where all initialization will happen.
DWORD WINAPI InitializeModThread(LPVOID hModule)
{
    // Initialize the logger first so we can capture all messages.
    // This could also be done in DllMain, but doing it here keeps
    // DllMain as minimal as possible.
    Logger::getInstance().setLogFile("CustomSensitivity.log");
    Logger::getInstance().setLevel(LogLevel::L_DEBUG); // Set a default log level

    LOG_DEBUG("Mod initialization thread started.");

    // Get the singleton instance of the ModManager and start it.
    // The ModManager's constructor and Start() method will handle
    // creating all components and installing all hooks.
    try
    {
        CustomSensitivity::ModManager::GetInstance().Start();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("An exception occurred during mod initialization: %s", e.what());
    }
    catch (...)
    {
        LOG_ERROR("An unknown exception occurred during mod initialization.");
    }

    return 0;
}

// The entry point for our DLL.
BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            // We don't want to receive thread attach/detach notifications,
            // which can help avoid deadlocks inside DllMain.
            DisableThreadLibraryCalls(hModule);

            // Create a new thread to initialize our mod.
            // This avoids blocking the game's main thread during startup.
            HANDLE hThread = CreateThread(nullptr, 0, InitializeModThread, hModule, 0, nullptr);
            if (hThread)
            {
                // We don't need to manage the thread ourselves, so we can close the handle.
                // The thread will continue to run in the background.
                CloseHandle(hThread);
            }
            break;
        }
        
        case DLL_PROCESS_DETACH:
        {
            // Shut down the mod manager, which will handle uninstalling all hooks
            // and cleaning up resources.
            CustomSensitivity::ModManager::GetInstance().Shutdown();
            break;
        }
    }
    return TRUE;
}