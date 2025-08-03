#pragma once
#include "Config.h"
#include "DebugData.h"

namespace CustomSensitivity
{
    class GameHook
    {
    public:
        GameHook(const Config& config, DebugData& debugData);
        bool Install();
        void Uninstall();

    private:
        bool CreateAndEnableHooks();

        // A struct to hold all game-specific pointers and addresses
        struct GamePointers
        {
            uintptr_t baseAddress = 0;
            uintptr_t thirdPersonHookAddress = 0;
            uintptr_t firstPersonHookAddress = 0;
            float* fov_address = nullptr;

            // --- Function Pointer Types ---
            // The type for the function we are hooking
            using t_ThirdPersonMouseSensitivity = __int64(__fastcall*)(int64_t a1, int64_t a2, float a3, float a4);
            using t_FirstPersonMouseSensitivity = void (__fastcall*)(int64_t a1, int64_t a2, int64_t a3, float a4, float a5);
            
            // Types for other game functions our hooks need to call
            using t_sub_391B78 = void(__fastcall*)(float* a1, float* a2);
            using t_sub_3915A8 = __int64(__fastcall*)(__int64 a1);

            // --- Function Pointers ---
            t_sub_391B78 sub_391B78 = nullptr;
            t_sub_3915A8 sub_3915A8 = nullptr;

            // --- Global Variable Pointers ---
            float* dword_39806BC = nullptr;
            float* dword_39806FC = nullptr;
        };

        GamePointers m_gamePtrs;
        const Config& m_config;
        DebugData& m_debugData;

        // --- Hooked Functions (Detours) ---
        // These are our replacement functions.
        static __int64 __fastcall ThirdPersonMouseSensitivity(int64_t a1, int64_t a2, float a3, float a4);
        static void __fastcall FirstPersonMouseSensitivity(__int64 a1, __int64 a2, __int64 a3, float a4, float a5);

        // --- Trampoline Pointers ---
        // This static variable will be set by MinHook to point to the original game function.
        static inline GamePointers::t_ThirdPersonMouseSensitivity m_original_ThirdPersonMouseSensitivity;
        static inline GamePointers::t_FirstPersonMouseSensitivity m_original_FirstPersonMouseSensitivity;
    };
}