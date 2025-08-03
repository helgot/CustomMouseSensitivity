#include "GameHook.h"
#include "Logger.h"
#include <Windows.h>
#include <MinHook.h>

namespace CustomSensitivity
{
    // A static pointer to the single instance of this class.
    // This is necessary so our static detour functions can access the class members.
    static GameHook* s_instance = nullptr;

    // --- Constants ---
    constexpr float kBaseFirstPersonFov = 55.0f;
    constexpr float kFirstPersonQuakeScale = 1.0f / 1.8227f;
    constexpr float kThirdPersonQuakeScale = 1.2225f * kFirstPersonQuakeScale;

    GameHook::GameHook(const Config& config, DebugData& debugData)
        : m_config(config), m_debugData(debugData)
    {
        s_instance = this;
        LOG_DEBUG("GameHook instance created.");

        // Initialize all game-specific pointers and addresses
        m_gamePtrs.baseAddress = (uintptr_t)GetModuleHandleA(NULL);
        LOG_DEBUG("Game base address: 0x%llx", m_gamePtrs.baseAddress);

        m_gamePtrs.thirdPersonHookAddress = m_gamePtrs.baseAddress + 0x395E18;
        m_gamePtrs.firstPersonHookAddress = m_gamePtrs.baseAddress + 0x395EE0;

        m_gamePtrs.sub_391B78 = reinterpret_cast<GamePointers::t_sub_391B78>(m_gamePtrs.baseAddress + 0x391B78);
        m_gamePtrs.sub_3915A8 = reinterpret_cast<GamePointers::t_sub_3915A8>(m_gamePtrs.baseAddress + 0x3915A8);

        m_gamePtrs.dword_39806BC = reinterpret_cast<float*>(m_gamePtrs.baseAddress + 0x39806BC);
        m_gamePtrs.dword_39806FC = reinterpret_cast<float*>(m_gamePtrs.baseAddress + 0x39806FC);

        m_debugData.fov_address = m_gamePtrs.baseAddress + 0x3EA0BE0;
    }

    bool GameHook::Install()
    {
        LOG_DEBUG("Installing game hooks...");
        return CreateAndEnableHooks();
    }

    void GameHook::Uninstall()
    {
        LOG_DEBUG("Uninstalling game hooks...");
        MH_DisableHook((LPVOID)m_gamePtrs.thirdPersonHookAddress);
        MH_RemoveHook((LPVOID)m_gamePtrs.thirdPersonHookAddress);
        MH_DisableHook((LPVOID)m_gamePtrs.firstPersonHookAddress);
        MH_RemoveHook((LPVOID)m_gamePtrs.firstPersonHookAddress);
    }

    bool GameHook::CreateAndEnableHooks()
    {
        // --- Third Person Sensitivity Hook ---
        if (MH_CreateHook((LPVOID)m_gamePtrs.thirdPersonHookAddress, &ThirdPersonMouseSensitivity, reinterpret_cast<LPVOID*>(&m_original_ThirdPersonMouseSensitivity)) != MH_OK) {
            LOG_ERROR("Failed to create hook for ThirdPersonMouseSensitivity.");
            return false;
        }
        if (MH_EnableHook((LPVOID)m_gamePtrs.thirdPersonHookAddress) != MH_OK) {
            LOG_ERROR("Failed to enable hook for ThirdPersonMouseSensitivity.");
            return false;
        }
        LOG_INFO("Hook for ThirdPersonMouseSensitivity created and enabled.");

        // --- First Person Sensitivity Hook ---
        if (MH_CreateHook((LPVOID)m_gamePtrs.firstPersonHookAddress, &FirstPersonMouseSensitivity, nullptr) != MH_OK) {
            LOG_ERROR("Failed to create hook for FirstPersonMouseSensitivity.");
            return false;
        }
        if (MH_EnableHook((LPVOID)m_gamePtrs.firstPersonHookAddress) != MH_OK) {
            LOG_ERROR("Failed to enable hook for FirstPersonMouseSensitivity.");
            return false;
        }
        LOG_INFO("Hook for FirstPersonMouseSensitivity created and enabled.");

        return true;
    }

    // --- Detour Implementations ---

    __int64 __fastcall GameHook::ThirdPersonMouseSensitivity(int64_t a1, int64_t a2, float a3, float a4)
    {
        // Use the static instance pointer to access member variables
        GameHook* pThis = s_instance;
        if (!pThis) {
             // This should never happen, but as a fallback, call the original function
            return m_original_ThirdPersonMouseSensitivity(a1, a2, a3, a4);
        }

        // Clear debug probes for this frame
        pThis->m_debugData.float_probe.clear();
        pThis->m_debugData.byte_probe.clear();
        pThis->m_debugData.dword_probe.clear();
        pThis->m_debugData.qword_probe.clear();

        float v5 = 0.0f; // The in-menu game slider will now have no contribution.
        float* v6 = *(float**)(a1 + 40);

        float aiming = *(float*)(a1 + 48); // 1.0f if aimed, 0.0f if not. (interpolated in transition)
        float aiming_scale = 1.0f + aiming * (pThis->m_config.third_person_aim_scale - 1.0f);
        float scale = pThis->m_config.third_person_sensitivity * aiming_scale * kThirdPersonQuakeScale;

        float v11, v12;
        v11 = (float)((float)((float)((float)(v6[12] - v6[11]) * v5) + v6[11]) * 0.017453292f) * *pThis->m_gamePtrs.dword_39806BC * scale; // x_scale
        v12 = (float)((float)((float)((float)(v6[14] - v6[13]) * v5) + v6[13]) * 0.017453292f) * *pThis->m_gamePtrs.dword_39806BC * scale; // y_scale

        pThis->m_gamePtrs.sub_391B78(&v11, &v12);

        *(float*)(a1 + 88) = v11 * a3;
        *(float*)(a1 + 92) = v12 * a4;

        return pThis->m_gamePtrs.sub_3915A8(a1);
    }

    void __fastcall GameHook::FirstPersonMouseSensitivity(__int64 a1, __int64 a2, __int64 a3, float a4, float a5)
    {
        GameHook* pThis = s_instance;
        if (!pThis) {
             // This should never happen, but as a fallback, call the original function
            return m_original_FirstPersonMouseSensitivity(a1, a2, a3, a4, a5);
        }

        // Clear debug probes
        pThis->m_debugData.float_probe.clear();
        pThis->m_debugData.byte_probe.clear();
        pThis->m_debugData.dword_probe.clear();
        pThis->m_debugData.qword_probe.clear();

        int v7; 
        if (*(char*)(a2 + 208) < 0)
            v7 = 0;
        else
            v7 = 1065353216;

        *(DWORD*)(a1 + 228) = v7;
        *(float*)(a1 + 168) = a4;
        *(float*)(a1 + 172) = a5;

        float v9 = 0.0f; // Zero out the in-game slider contribution

        bool is_aiming = (*(BYTE*)(a2 + 211) & 8) == 0x8;
        float aiming_scale = 1.0f + (is_aiming ? 1.0f : 0.0f) * (pThis->m_config.first_person_aim_scale - 1.0f);

        float fov = *reinterpret_cast<float*>(pThis->m_debugData.fov_address);
        float fov_scale = pThis->m_config.scale_first_person_sensitivity_with_fov ? fov / kBaseFirstPersonFov : 1.0f;
        float scale = pThis->m_config.first_person_sensitivity * aiming_scale * kFirstPersonQuakeScale / fov_scale;

        // Fix scaling to be consistent across non-scoped and scoped modes.
        *(float*)(a1 + 84) = 190.0f;
        *(float*)(a1 + 76) = 250.0f;

        float x_sens = (float)((float)((float)((float)(*(float*)(a1 + 80) - *(float*)(a1 + 76)) * v9) + *(float*)(a1 + 76)) * 0.017453292f) * *pThis->m_gamePtrs.dword_39806FC * scale;
        float y_sens = (float)((float)((float)((float)(*(float*)(a1 + 88) - *(float*)(a1 + 84)) * v9) + *(float*)(a1 + 84)) * 0.017453292f) * *pThis->m_gamePtrs.dword_39806FC * scale;

        *(float*)(a2 + 128) = (a4 * *(float*)(a1 + 240)) * x_sens;
        float final_y = (a5 * *(float*)(a1 + 244)) * y_sens;

        // Clamp vertical rotation
        *(float*)(a2 + 136) = fminf(3.1415927f, fmaxf(-3.1415927f, final_y));
    }
}