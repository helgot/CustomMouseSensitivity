#include "mod.h"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <Windows.h>
#include <imgui.h>
#include <MinHook.h>
#include <winnt.h>
#include <json.hpp>

#include "logger.h"
#include "renderhook_vulkan.h"

#include "config.h"
#include "debug_data.h"
#include "ui.h"

namespace CustomSensitivity
{
    constexpr float kBaseFirstPersonFov = 55.0f;
    constexpr float kFirstPersonQuakeScale = 1.0f / 1.8227f;
    constexpr float kThirdPersonQuakeScale = 1.2225f * kFirstPersonQuakeScale;


    /* These are made global to the namespace, as they need to be accessible by the callbacks.*/
    Config g_config {};
    DebugData g_debugData {};

    void __fastcall ImGuiDrawCommandsCallback()
    {
        if (!g_debugData.show_menu)
            return;

        ImGui::Begin("Custom Sensitivity (Display Toggle: F10)");
        ImGuiDrawConfigManagementHelper(g_config);

        if (ImGui::BeginTabBar("InputConfigTabs"))
        {
            if (ImGui::BeginTabItem("Config"))
            {
                ImGuiDrawControlConfigHelper(g_config);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Debug"))
            {
                ImGuiDrawDebugHelper(g_debugData);
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    void __fastcall ImGuiHandleInputCallback()
    {
        static bool F10PressedLastFrame = false;
        bool F10PressedThisFrame = GetAsyncKeyState(VK_F10) & 0x8000;

        if (F10PressedThisFrame && !F10PressedLastFrame)
        {
            g_debugData.show_menu = !g_debugData.show_menu;

            ImGuiIO& io = ImGui::GetIO(); 
            io.MouseDrawCursor = g_debugData.show_menu;
        }

        F10PressedLastFrame = F10PressedThisFrame;
    }

    namespace Target {
        using t_sub_391B78 = void (__fastcall*)(float* a1, float* a2);
        using t_sub_3915A8 = __int64(__fastcall*)(__int64 a1);

        // Function pointers  to game functions. 
        t_sub_391B78 sub_391B78;
        t_sub_3915A8 sub_3915A8;

        // Pointers to the global game variables.
        int64_t* dword_39806BC;
        int64_t* dword_39806FC;
    }

    /*
        Third-person aim sensitivity function:
        Supports setting the free-aim sensitivity, as well as the aiming sensitivity.
    */
    __int64 __fastcall ThirdPersonMouseSensitivity(int64_t a1, int64_t a2, float a3, float a4)
    {

        g_debugData.float_probe = {};
        g_debugData.byte_probe = {};
        g_debugData.dword_probe = {};
        g_debugData.qword_probe = {};

        float v5 = 0.0f; // xmm0_4
        float* v6; // rax
        float v7 = 0.0f; // xmm4_4
        float v8 = 0.0f; // xmm0_4
        float v9 = 0.0f; // xmm1_4
        float v11 = 0.0f; // [rsp+60h] [rbp+18h] BYREF
        float v12 = 0.0f; // [rsp+68h] [rbp+20h] BYREF

        *(float*)(a1 + 80) = a3;
        *(float*)(a1 + 84) = a4;
        v5 = 0.0f; // The in-menu game slider will now have no contribution.
        //v5 = Target::sub_3ADEA8(a1);
        v6 = *(float**)(a1 + 40);
        v7 = v5;

        float aiming = *(float*)(a1 + 48); // 1.0f if aimed, 0.0f if not. (interpolated in transition)
        float aiming_scale = 1 + aiming*(g_config.third_person_aim_scale - 1);
        float scale = g_config.third_person_sensitivity * aiming_scale * kThirdPersonQuakeScale;

        v8 = (float)((float)((float)((float)(v6[14] - v6[13]) * v5) + v6[13]) * 0.017453292) * *(float*)Target::dword_39806BC * scale; // y_scale
        v11 = (float)((float)((float)((float)(v6[12] - v6[11]) * v5) + v6[11]) * 0.017453292) * *(float*)Target::dword_39806BC * scale; // x_scale
        v12 = v8;
        Target::sub_391B78(&v11, &v12);
        v9 = v12 * a4;
        *(float*)(a1 + 88) = v11 * a3;
        *(float*)(a1 + 92) = v9;
        return Target::sub_3915A8(a1);
    }

    /*
        First-person aim sensitivity function:
    */
    void __fastcall FirstPersonMouseSensitivity(__int64 a1, __int64 a2, __int64 a3, float a4, float a5)
    {
        g_debugData.float_probe = {};
        g_debugData.byte_probe = {};
        g_debugData.dword_probe = {};
        g_debugData.qword_probe = {};

        int v7; // xmm0_4
        float v8; // xmm6_4
        float v9; // xmm4_4
        float v10; // xmm0_4
        float v11; // xmm0_4
        float v12; // xmm6_4
        float v13; // [rsp+68h] [rbp+20h] BYREF

        if (*(char*)(a2 + 208) < 0)
            v7 = 0;
        else
            v7 = 1065353216;

        v8 = a5;
        *(float*)(a1 + 172) = a5;
        *(DWORD*)(a1 + 228) = v7;
        *(float*)(a1 + 168) = a4;
        v9 = 0.0f; // gives the in-game mouse slider value zero contribution to the sensitivity.

        g_debugData.float_probe.insert({ "v9", v9});
        // ADS check value. 0x8 -> ADS
        g_debugData.byte_probe.insert({ "*(BYTE*)(a2 + 211)", *(BYTE*)(a2 + 211)});
        g_debugData.float_probe.insert({ "(a1 + 88)", *(float*)(a1 + 88) });
        g_debugData.float_probe.insert({ "(a1 + 84)", *(float*)(a1 + 84) });
        g_debugData.float_probe.insert({ "(a1 + 80)", *(float*)(a1 + 80) });
        g_debugData.float_probe.insert({ "(a1 + 76)", *(float*)(a1 + 76) });
        g_debugData.float_probe.insert({ "(a1 + 48)", *(float*)(a1 + 48) });
        g_debugData.float_probe.insert({ "* (float*)Target::dword_39806FC", *(float*)Target::dword_39806FC });

        float aiming = 1.0f ? (*(BYTE*)(a2 + 211) & 8) == 0x8 : 0.0f; 
        float aiming_scale = 1 + aiming*(g_config.first_person_aim_scale- 1);

        // fov_scale applies a scaling to 'undo' the games scaling of the rotation it does some where else!
        float fov_scale = !g_config.scale_first_person_sensitivity_with_fov ? kBaseFirstPersonFov / *reinterpret_cast<float*>(g_debugData.fov_address) : 1.0f;
        float scale = g_config.first_person_sensitivity * aiming_scale * fov_scale * kFirstPersonQuakeScale;

        // This fixes the scaling to be the consistent across the non-scoped and scoped modes.
        *(float*)(a1 + 84) = 190.0f; 
        *(float*)(a1 + 76) = 250.0f; 

        v10 = (float)((float)((float)((float)(*(float*)(a1 + 88) - *(float*)(a1 + 84)) * v9) + *(float*)(a1 + 84)) * 0.017453292) * *(float*)Target::dword_39806FC * scale;
        a5 = (float)((float)((float)((float)(*(float*)(a1 + 80) - *(float*)(a1 + 76)) * v9) + *(float*)(a1 + 76)) * 0.017453292) * *(float*)Target::dword_39806FC * scale;
        v13 = v10;

        v11 = -3.1415927;
        *(float*)(a2 + 128) = (float)(a4 * *(float*)(a1 + 240)) * a5;

        v12 = (float)(v8 * *(float*)(a1 + 244)) * v13;

        if (v12 >= -3.1415927)
            v11 = fminf(v12, 3.1415927);
        *(float*)(a2 + 136) = v11;

        g_debugData.float_probe.insert({ "(a2 + 128)", *(float*)(a2 + 128) });
        g_debugData.float_probe.insert({ "(a2 + 136)", *(float*)(a2 + 136) });
        g_debugData.float_probe.insert({ "(a1 + 240)", *(float*)(a1 + 240) });
        g_debugData.float_probe.insert({ "(a1 + 244)", *(float*)(a1 + 244) });
        g_debugData.float_probe.insert({ "*(float*)(a1 + 172)", *(float*)(a1 + 172) });
        g_debugData.float_probe.insert({ "*(float*)(a1 + 168)", *(float*)(a1 + 168) });
        g_debugData.dword_probe.insert({ "*(DWORD*)(a1 + 228)", *(DWORD*)(a1 + 228) });
    }

    bool Mod::CreateHooks()
    {
        for(const auto& hook: m_functionHooks)
        {
            if(MH_CreateHook((LPVOID)hook.original, (LPVOID)hook.detour, (LPVOID*)&hook.trampoline) == MH_OK)
            {
                LOG_INFO("%s: Creation of hook for %llx successful.", __func__, hook.original);
            }
            else {
                LOG_INFO("%s: Creation of hook for %llx failed.", __func__, hook.original);
                return false;
            }
        }
        return true; 
    }

    bool Mod::EnableHooks()
    {
        for(const auto& hook: m_functionHooks)
        {
            if(MH_EnableHook((LPVOID)hook.original) == MH_OK)
            {
                LOG_INFO("%s: Enabling of hook for %llx successful.", __func__, hook.original);
            }
            else {
                LOG_INFO("%s: Enabling of hook for %llx failed.", __func__, hook.original);
                return false;
            }
        }
        return true; 
    }
    
    bool Mod::HookGameFunctions()
    {
        g_debugData.baseAddress = (uintptr_t)GetModuleHandleA(NULL);
        LOG_DEBUG("Base address: 0x%llx", g_debugData.baseAddress);

        Target::sub_391B78 = reinterpret_cast<Target::t_sub_391B78>(g_debugData.baseAddress + 0x391B78);
        Target::sub_3915A8 = reinterpret_cast<Target::t_sub_3915A8>(g_debugData.baseAddress + 0x3915A8);

        Target::dword_39806BC = (int64_t*)(g_debugData.baseAddress + 0x39806BC);
        Target::dword_39806FC = (int64_t*)(g_debugData.baseAddress + 0x39806FC);

        g_debugData.thirdPersonHookAddress = g_debugData.baseAddress + 0x395E18;
        g_debugData.firstPersonHookAddress = g_debugData.baseAddress + 0x395EE0;

        m_functionHooks = 
        {
            {reinterpret_cast<void*>(g_debugData.thirdPersonHookAddress), &ThirdPersonMouseSensitivity, nullptr},
            {reinterpret_cast<void*>(g_debugData.firstPersonHookAddress), &FirstPersonMouseSensitivity, nullptr},
        };

        if(!CreateHooks())
        {
            LOG_ERROR("%s: Failed to create hooks.", __func__);
            return false;
        }

        if(!EnableHooks())
        {
            LOG_ERROR("%s: Failed to enable hooks.", __func__);
            return false;
        }

        g_debugData.fov_address = g_debugData.baseAddress + 0x3EA0BE0;

        g_debugData.show_menu = g_config.show_menu_on_start_up;
        return true; 
    }

    void Mod::UnhookGameFunctions()
    {
        MH_DisableHook((LPVOID)g_debugData.thirdPersonHookAddress);
        MH_RemoveHook((LPVOID)g_debugData.thirdPersonHookAddress);
        MH_DisableHook((LPVOID)g_debugData.firstPersonHookAddress);
        MH_RemoveHook((LPVOID)g_debugData.firstPersonHookAddress);
        LOG_DEBUG("Game functions unhooked.");
    }
    
    bool Mod::LoadConfig() 
    {
        bool no_errors_in_config = true;
        g_config = {};
        if(!g_config.LoadConfigFromFile(kConfigFileName))
        {
            g_config.LoadDefault();
            LOG_ERROR("%s, config contains errors. Loading default configuration.");
            no_errors_in_config = false;
        }

       
        std::string debug_config_string = nlohmann::json(g_config).dump();
        LOG_INFO("Loaded Config: %s", debug_config_string.c_str());

        /* 
            Attempt to set the log level using the log level string specified in the .json config.
            If this fails to resolve to a log level, set the log level to L_INFO.
        */
        auto log_level = from_string<LogLevel>(g_config.log_level);
        Logger::getInstance().setLevel(log_level.has_value() ? log_level.value() : LogLevel::L_INFO);
        return no_errors_in_config;
    }

    Mod& Mod::GetInstance()
    {
        LOG_DEBUG("Mod instance accessed.");
        static Mod instance;
        return instance;
    }

    Mod::Mod()
    {
        LOG_DEBUG("Mod instance created.");
        LoadConfig();
        Logger::getInstance().setLogFile("CustomSensivitity.log");
        m_rendererHook = std::make_unique<RenderHookVulkan>();
    }

    Mod::~Mod()
    {
        Shutdown();
        LOG_DEBUG("Mod instance destroyed.");
    }

    void Mod::Start()
    {
        if (!m_rendererHook) return;

        m_rendererHook->SetGUICallback(ImGuiDrawCommandsCallback);
        m_rendererHook->SetHandleInputCallback(ImGuiHandleInputCallback);

        if (MH_Initialize() != MH_OK)
        {
            LOG_ERROR("Failed to initialize MinHook.");
            return;
        } 

        LOG_DEBUG("MinHook initialized successfully.");


        if(!m_rendererHook->HookGraphicsAPI())
        {
            LOG_ERROR("Failed to Hook Graphics.");
        }
        else 
        {
            LOG_DEBUG("Graphics API Hooked successfully.");
        } 

        HookGameFunctions();
    }

    void Mod::Shutdown()
    {
        if (m_rendererHook)
        {
            m_rendererHook->UnhookGraphicsAPI();
        }
        UnhookGameFunctions();

        if (MH_Uninitialize() != MH_OK)
        {
            LOG_ERROR("Failed to uninitialize MinHook.");
        }
        else
        {
            LOG_DEBUG("MinHook uninitialized.");
        }
    }
}