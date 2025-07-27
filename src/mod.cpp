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
#include "game_data.h"

namespace CustomSensitivity
{
    constexpr float kMinSliderValue = 0.0f;
    constexpr float kMaxSliderValue = 10.0f;
    constexpr float kDefaultValue = 1.0f; 
    constexpr float kBaseFirstPersonFov = 55.0f;
    constexpr float kFirstPersonQuakeScale = 1.0f / 1.8227f;
    constexpr float kThirdPersonQuakeScale = 1.2225f * kFirstPersonQuakeScale;
    constexpr std::string_view kConfigFileName = "CustomSensitivity.json";

    Config g_config {};
    GameData g_gameData {};

    template<class T>
    void ImGuiPrintMapHelper(const std::unordered_map<std::string_view, T>& map)
    {
        if (map.empty())
        {
            ImGui::Text("map is null.");
            return;
        }

        for (const auto& [key, value] : map)
        {
            if constexpr (std::is_same_v<T, int>)
            {
                ImGui::Text("   %s: %d", key.data(), value);
            }
            else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>)
            {
                ImGui::Text("   %s: %.3f", key.data(), static_cast<double>(value));
            }
            else if constexpr (std::is_same_v<T, uint8_t>)
            {
                ImGui::Text("   %s: 0x%02X", key.data(), value);
            }
            else if constexpr (std::is_same_v<T, uint32_t>)
            {
                ImGui::Text("   %s: 0x%08X", key.data(), value);
            }
            else if constexpr (std::is_same_v<T, BYTE>)
            {
                ImGui::Text("   %s: 0x%02X", key.data(), value);
            }
            else
            {
                ImGui::Text("   %s: [unsupported type]", key.data());
            }
        }
    }

    void ImGuiDrawControlConfigHelper()
    {
        ImGui::SliderFloat("First-Person Sensitivity", &g_config.first_person_sensitivity, kMinSliderValue, kMaxSliderValue);
        ImGui::SliderFloat("First-Person Aim Scale", &g_config.first_person_aim_scale, kMinSliderValue, kMaxSliderValue);
        ImGui::SliderFloat("Third-Person Sensitivity", &g_config.third_person_sensitivity, kMinSliderValue, kMaxSliderValue);
        ImGui::SliderFloat("Third-Person Aim Scale", &g_config.third_person_aim_scale, kMinSliderValue, kMaxSliderValue);
        ImGui::Checkbox("Scale First-Person Sensitivity With FOV", &g_config.scale_first_person_sensitivity_with_fov);
        ImGui::Checkbox("Show Menu On Start-up ", &g_config.show_menu_on_start_up);
    }

    void __fastcall ImGuiDrawConfigManagementHelper()
    {
        ImGui::Separator();
        if (ImGui::Button("Save Config"))
        {
            g_config.SaveConfigToFile(kConfigFileName);
        }

        ImGui::SameLine();

        if (ImGui::Button("Load Config"))
        {
            g_config.LoadConfigFromFile(kConfigFileName);
        }

        ImGui::SameLine();

        if (ImGui::Button("Set Default"))
        {
            g_config.LoadDefault();
        }
    }

    void ImGuiDrawDebugHelper()
    {
        ImGui::Text("FOV: %0.2f", *reinterpret_cast<float*>(g_gameData.fov_address));
        ImGui::Separator();
        ImGui::Text("Debug Probes:");
        ImGui::Separator();
        ImGuiPrintMapHelper(g_gameData.float_probe);
        ImGui::Separator();
        ImGuiPrintMapHelper(g_gameData.byte_probe);
        ImGui::Separator();
        ImGuiPrintMapHelper(g_gameData.dword_probe);
        ImGui::Separator();
        ImGuiPrintMapHelper(g_gameData.qword_probe);
        ImGui::Separator();
        ImGuiPrintMapHelper(g_gameData.int_probe);
        ImGui::Separator();
    }


    void __fastcall ImGuiDrawCommandsCallback()
    {
        if (!g_gameData.show_menu)
            return;

        ImGui::Begin("Custom Sensitivity (Display Menu Toggle: F10)");
        ImGuiDrawConfigManagementHelper();

        if (ImGui::BeginTabBar("InputConfigTabs"))
        {
            if (ImGui::BeginTabItem("Config"))
            {
                ImGuiDrawControlConfigHelper();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Debug"))
            {
                ImGuiDrawDebugHelper();
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
            g_gameData.show_menu = !g_gameData.show_menu;

            ImGuiIO& io = ImGui::GetIO(); 
            io.MouseDrawCursor = g_gameData.show_menu;
        }

        F10PressedLastFrame = F10PressedThisFrame;
    }

    namespace Target {

        using t_sub_4162B0 = bool(__fastcall*)(void* a2);
        using t_sub_1461C0 = bool(__fastcall*)();
        using t_sub_2D32BE0 = void(__fastcall*)(void* buf);
        using t_sub_263DAF4 = double(__fastcall*)(uintptr_t arg1, uintptr_t arg2, int arg3, int arg4, int arg5);
        using t_sub_3ADEA8 = float(__fastcall*)(__int64 a1);
        using t_sub_391B78 = void (__fastcall*)(float* a1, float* a2);
        using t_sub_3915A8 = __int64(__fastcall*)(__int64 a1);
        using t_sub_3923E4 = float(__fastcall*)(__int64 a1);
        using t_sub_3ADF20 = float (__fastcall*)(__int64 a1, char a2);
        using t_sub_173550 = __int64 (*)();
        using t_sub_1B5C09C = char(__fastcall*)(__int64 a1, int a2);
        using t_sub_112F1D8 = __m128 (__fastcall*)(double a1, char a2);
        using t_sub_1489360 = float(__fastcall*)(float** a1, float a2, float a3, unsigned int a4);
        using t_sub_2C3F13C = double(__fastcall*)();
        using t_sub_11A5EBC = bool(__fastcall*)(__int64 a1, char a2);
        using t_sub_2C3F0FC = float(__fastcall*)(float a1);
        using t_sub_3B83FC = __m128 (__fastcall*)(float a1);

        // Function pointers to be initialized
        t_sub_4162B0  sub_4162B0;
        t_sub_1461C0  sub_1461C0;
        t_sub_2D32BE0 sub_2D32BE0;
        t_sub_263DAF4 sub_263DAF4;
        t_sub_3ADEA8 sub_3ADEA8;
        t_sub_391B78 sub_391B78;
        t_sub_3915A8 sub_3915A8;
        t_sub_3923E4 sub_3923E4;
        t_sub_3ADF20 sub_3ADF20;
        t_sub_173550 sub_173550;
        t_sub_1B5C09C sub_1B5C09C;
        t_sub_112F1D8 sub_112F1D8;
        t_sub_1489360 sub_1489360;
        t_sub_2C3F13C sub_2C3F13C;
        t_sub_11A5EBC sub_11A5EBC;
        t_sub_2C3F0FC sub_2C3F0FC;
        t_sub_3B83FC sub_3B83FC;

        // Pointers to the global variables
        int64_t* dword_39ECE40;
        int64_t* dword_39806BC;
        int64_t* dword_3973128;
        int64_t* dword_39806FC;
        int64_t* qword_39ECE48;
        int64_t* qword_571A380;
    }

    /*
        Third-person aim sensitivity function:
        Supports setting the free-aim sensitivity, as well as the aiming sensitivity.
    */
    __int64 __fastcall ThirdPersonMouseSensitivity(int64_t a1, int64_t a2, float a3, float a4)
    {

        g_gameData.float_probe = {};
        g_gameData.byte_probe = {};
        g_gameData.dword_probe = {};
        g_gameData.qword_probe = {};

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
        g_gameData.float_probe = {};
        g_gameData.byte_probe = {};
        g_gameData.dword_probe = {};
        g_gameData.qword_probe = {};

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
        v9 = 0.0f; // The in-menu game slider will now have no contribution.

        g_gameData.float_probe.insert({ "v9", v9});
        // ADS check value. 0x8 -> ADS
        g_gameData.byte_probe.insert({ "*(BYTE*)(a2 + 211)", *(BYTE*)(a2 + 211)});
        g_gameData.float_probe.insert({ "(a1 + 88)", *(float*)(a1 + 88) });
        g_gameData.float_probe.insert({ "(a1 + 84)", *(float*)(a1 + 84) });
        g_gameData.float_probe.insert({ "(a1 + 80)", *(float*)(a1 + 80) });
        g_gameData.float_probe.insert({ "(a1 + 76)", *(float*)(a1 + 76) });
        g_gameData.float_probe.insert({ "(a1 + 48)", *(float*)(a1 + 48) });
        g_gameData.float_probe.insert({ "* (float*)Target::dword_39806FC", *(float*)Target::dword_39806FC });

        float aiming = 1.0f ? (*(BYTE*)(a2 + 211) & 8) == 0x8 : 0.0f; 
        float aiming_scale = 1 + aiming*(g_config.first_person_aim_scale- 1);

        // fov_scale applies a scaling to 'undo' the games scaling of the rotation it does some where else!
        float fov_scale = !g_config.scale_first_person_sensitivity_with_fov ? kBaseFirstPersonFov / *reinterpret_cast<float*>(g_gameData.fov_address) : 1.0f;
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

        g_gameData.float_probe.insert({ "(a2 + 128)", *(float*)(a2 + 128) });
        g_gameData.float_probe.insert({ "(a2 + 136)", *(float*)(a2 + 136) });
        g_gameData.float_probe.insert({ "(a1 + 240)", *(float*)(a1 + 240) });
        g_gameData.float_probe.insert({ "(a1 + 244)", *(float*)(a1 + 244) });
        g_gameData.float_probe.insert({ "*(float*)(a1 + 172)", *(float*)(a1 + 172) });
        g_gameData.float_probe.insert({ "*(float*)(a1 + 168)", *(float*)(a1 + 168) });
        g_gameData.dword_probe.insert({ "*(DWORD*)(a1 + 228)", *(DWORD*)(a1 + 228) });
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
        g_gameData.baseAddress = (uintptr_t)GetModuleHandleA(NULL);
        LOG_DEBUG("Base address: 0x%llx", g_gameData.baseAddress);

        Target::sub_4162B0 = reinterpret_cast<Target::t_sub_4162B0>(g_gameData.baseAddress + 0x4162B0);
        Target::sub_2D32BE0 = reinterpret_cast<Target::t_sub_2D32BE0>(g_gameData.baseAddress + 0x2D32BE0);
        Target::sub_263DAF4 = reinterpret_cast<Target::t_sub_263DAF4>(g_gameData.baseAddress + 0x263DAF4);
        Target::sub_3ADEA8 = reinterpret_cast<Target::t_sub_3ADEA8>(g_gameData.baseAddress + 0x3ADEA8);
        Target::sub_391B78 = reinterpret_cast<Target::t_sub_391B78>(g_gameData.baseAddress + 0x391B78);
        Target::sub_3915A8 = reinterpret_cast<Target::t_sub_3915A8>(g_gameData.baseAddress + 0x3915A8);
        Target::sub_3923E4 = reinterpret_cast<Target::t_sub_3923E4>(g_gameData.baseAddress + 0x3923E4);
        Target::sub_3ADF20 = reinterpret_cast<Target::t_sub_3ADF20>(g_gameData.baseAddress + 0x3ADF20);
        Target::sub_173550 = reinterpret_cast<Target::t_sub_173550>(g_gameData.baseAddress + 0x173550);

        Target::sub_1B5C09C = reinterpret_cast<Target::t_sub_1B5C09C>(g_gameData.baseAddress + 0x1B5C09C);
        Target::sub_112F1D8 = reinterpret_cast<Target::t_sub_112F1D8>(g_gameData.baseAddress + 0x112F1D8);
        Target::sub_1489360 = reinterpret_cast<Target::t_sub_1489360>(g_gameData.baseAddress + 0x1489360);
        Target::sub_2C3F13C = reinterpret_cast<Target::t_sub_2C3F13C>(g_gameData.baseAddress + 0x2C3F13C);
        Target::sub_11A5EBC = reinterpret_cast<Target::t_sub_11A5EBC>(g_gameData.baseAddress + 0x11A5EBC);
        Target::sub_2C3F0FC = reinterpret_cast<Target::t_sub_2C3F0FC>(g_gameData.baseAddress + 0x2C3F0FC);
        Target::sub_3B83FC = reinterpret_cast<Target::t_sub_3B83FC>(g_gameData.baseAddress + 0x3B83FC);

        Target::qword_571A380 = (int64_t*)(g_gameData.baseAddress + 0x571A380);
        Target::dword_39806BC = (int64_t*)(g_gameData.baseAddress + 0x39806BC);
        Target::dword_39806FC = (int64_t*)(g_gameData.baseAddress + 0x39806FC);
        Target::dword_3973128 = (int64_t*)(g_gameData.baseAddress + 0x3973128);
        Target::dword_39ECE40 = (int64_t*)(g_gameData.baseAddress + 0x39ECE40);
        Target::qword_39ECE48 = (int64_t*)(g_gameData.baseAddress + 0x39ECE48);

        g_gameData.thirdPersonHookAddress = g_gameData.baseAddress + 0x395E18;
        g_gameData.firstPersonHookAddress = g_gameData.baseAddress + 0x395EE0;

        m_functionHooks = 
        {
            {reinterpret_cast<void*>(g_gameData.thirdPersonHookAddress), &ThirdPersonMouseSensitivity, nullptr},
            {reinterpret_cast<void*>(g_gameData.firstPersonHookAddress), &FirstPersonMouseSensitivity, nullptr},
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

        g_gameData.fov_address = g_gameData.baseAddress + 0x3EA0BE0;

        // Load config from file
        if(!g_config.LoadConfigFromFile(kConfigFileName))
        {
            g_config.LoadDefault();
            g_config.SaveConfigToFile(kConfigFileName);
        }

        // Print the loaded configuration:
        {
            nlohmann::json json_print;
            json_print = g_config;
            std::string debug_json_print = json_print.dump();
            LOG_INFO("Loaded Config: %s", debug_json_print.c_str());
        }

        g_gameData.show_menu = g_config.show_menu_on_start_up;
        return true; 
    }

    void Mod::UnhookGameFunctions()
    {
        MH_DisableHook((LPVOID)g_gameData.thirdPersonHookAddress);
        MH_RemoveHook((LPVOID)g_gameData.thirdPersonHookAddress);
        MH_DisableHook((LPVOID)g_gameData.firstPersonHookAddress);
        MH_RemoveHook((LPVOID)g_gameData.firstPersonHookAddress);
        LOG_DEBUG("Game functions unhooked.");
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
        Logger::getInstance().setLevel(LogLevel::L_INFO);
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