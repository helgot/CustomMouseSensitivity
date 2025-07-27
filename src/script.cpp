#include "script.h"

#include "logger.h"

#include <cstdint>
#include <exception>
#include <unordered_map>
#include <Windows.h>
#include <imgui.h>
#include <MinHook.h>
#include <winnt.h>
#include <json.hpp>
#include <fstream>


namespace Mod
{
    struct Config
    {
        float first_person_sensitivity = 1.0f;
        float first_person_aim_scale = 1.0f;
        float third_person_sensitivity = 1.0f;
        float third_person_aim_scale = 1.0f;
        bool scale_first_person_sensitivity_with_fov = true;
        bool show_menu_on_start_up = true;
    };

    constexpr float kMinSliderValue = 0.0f;
    constexpr float kMaxSliderValue = 10.0f;
    constexpr float kDefaultValue = 1.0f; 
    constexpr std::string_view kConfigFileName = "CustomSensitivity.json";

    void to_json(nlohmann::json& j, const Config& config)
    {
        j = {
            { "first_person_sensitivity", config.first_person_sensitivity },
            { "first_person_aim_scale", config.first_person_aim_scale },
            { "third_person_sensitivity", config.third_person_sensitivity },
            { "third_person_aim_scale", config.third_person_aim_scale },
            { "scale_first_person_sensitivity_with_fov", config.scale_first_person_sensitivity_with_fov },
            { "show_menu_on_start_up", config.show_menu_on_start_up}
        };
    }

    void from_json(const nlohmann::json& j, Config& config)
    {
        j.at("first_person_sensitivity").get_to(config.first_person_sensitivity);
        j.at("first_person_aim_scale").get_to(config.first_person_aim_scale);
        j.at("third_person_sensitivity").get_to(config.third_person_sensitivity);
        j.at("third_person_aim_scale").get_to(config.third_person_aim_scale);
        j.at("scale_first_person_sensitivity_with_fov").get_to(config.scale_first_person_sensitivity_with_fov);
        j.at("show_menu_on_start_up").get_to(config.show_menu_on_start_up);
    }

    bool SaveConfigToFile(const Mod::Config& config, std::string_view filepath)
    {
        try {
            std::ofstream file(std::string(filepath).c_str());
            if (!file) return false;
            nlohmann::json j = config;
            file << j.dump(4);
            return true;
        } catch (const std::exception& e){
            LOG_DEBUG(e.what());
            return false;
        }
    }

    void LoadDefault(Mod::Config& config)
    {
        config = {};
    }

    bool LoadConfigFromFile(Mod::Config& config, std::string_view filepath)
    {
        try {
            std::ifstream file(std::string(filepath).c_str());
            if (!file) 
            {
                LoadDefault(config);
                return false;
            }
            nlohmann::json j;
            file >> j;
            config = j.get<Mod::Config>();
            return true;
        } catch (const std::exception& e){
            LOG_DEBUG(e.what());
            LoadDefault(config);
            return false;
        }
    }
}


namespace GameData
{
    uintptr_t baseAddress = 0;
    uintptr_t thirdPersonHookAddress = 0;
    uintptr_t firstPersonHookAddress = 0;
    uintptr_t fov_address = 0;
    bool show_menu = true;

    Mod::Config config {
        .first_person_sensitivity = 1.0f,
        .first_person_aim_scale = 1.0f,
        .third_person_sensitivity = 1.0f,
        .third_person_aim_scale = 1.0f,
        .scale_first_person_sensitivity_with_fov = true,
        .show_menu_on_start_up = true,
    };
   
    constexpr float base_first_person_fov = 55.0f;
    constexpr float first_person_quake_scale = 1.0f / 1.8227f;
    constexpr float third_person_quake_scale = 1.2225f * first_person_quake_scale;

    std::unordered_map<std::string_view, int> int_probe;
    std::unordered_map<std::string_view, float> float_probe;
    std::unordered_map<std::string_view, DWORD> dword_probe;
    std::unordered_map<std::string_view, BYTE> byte_probe;
    std::unordered_map<std::string_view, uint64_t> qword_probe;
}

template<class T>
void PrintMap(const std::unordered_map<std::string_view, T>& map)
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

void DrawControlConfig(Mod::Config& config)
{
    ImGui::SliderFloat("First-Person Sensitivity", &config.first_person_sensitivity, Mod::kMinSliderValue, Mod::kMaxSliderValue);
    ImGui::SliderFloat("First-Person Aim Scale", &config.first_person_aim_scale, Mod::kMinSliderValue, Mod::kMaxSliderValue);
    ImGui::SliderFloat("Third-Person Sensitivity", &config.third_person_sensitivity, Mod::kMinSliderValue, Mod::kMaxSliderValue);
    ImGui::SliderFloat("Third-Person Aim Scale", &config.third_person_aim_scale, Mod::kMinSliderValue, Mod::kMaxSliderValue);
    ImGui::Checkbox("Scale First-Person Sensitivity With FOV", &config.scale_first_person_sensitivity_with_fov);
    ImGui::Checkbox("Show Menu On Start-up ", &GameData::config.show_menu_on_start_up);

}

void __fastcall ImGuiDrawConfigManagement()
{
    ImGui::Separator();
    if (ImGui::Button("Save Config"))
    {
        Mod::SaveConfigToFile(GameData::config, Mod::kConfigFileName);
    }

    ImGui::SameLine();

    if (ImGui::Button("Load Config"))
    {
        Mod::LoadConfigFromFile(GameData::config, Mod::kConfigFileName);
    }

    ImGui::SameLine();

    if (ImGui::Button("Set Default"))
    {
        Mod::LoadDefault(GameData::config);
    }
}

void DrawDebug()
{
    ImGui::Text("FOV: %0.2f", *reinterpret_cast<float*>(GameData::fov_address));
    ImGui::Separator();
    ImGui::Text("Debug Probes:");
    ImGui::Separator();
    PrintMap(GameData::float_probe);
    ImGui::Separator();
    PrintMap(GameData::byte_probe);
    ImGui::Separator();
    PrintMap(GameData::dword_probe);
    ImGui::Separator();
    PrintMap(GameData::qword_probe);
    ImGui::Separator();
    PrintMap(GameData::int_probe);
    ImGui::Separator();
}


void __fastcall ImGuiDrawCommandsCallback()
{
    if (!GameData::show_menu)
        return;

    ImGui::Begin("Custom Sensitivity (Display Menu Toggle: F10)");
    ImGuiDrawConfigManagement();

    if (ImGui::BeginTabBar("InputConfigTabs"))
    {
        if (ImGui::BeginTabItem("Config"))
        {
            DrawControlConfig(GameData::config);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Debug"))
        {
            DrawDebug();
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
        GameData::show_menu= !GameData::show_menu;

        ImGuiIO& io = ImGui::GetIO(); 
        io.MouseDrawCursor = GameData::show_menu;
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

float __fastcall sub_3ADEA8(__int64 a1)
{
    float v1;
    float v2;
    float v3;
    float v4;
    float v6;

    v1 = 0.0;
    v2 = (float)*Target::dword_3973128 * 0.050000001;
    if (v2 >= 0.0)
        v3 = fminf(v2, 1.0);
    else
        v3 = 0.0;
    v4 = (float)*Target::dword_3973128 * 0.050000001;
    if (v4 >= 0.0)
        v1 = fminf(v4, 1.0);
    v6 = (float)((float)(v1 - v3) * *(float*)(a1 + 48)) + v3;
    Target::sub_3923E4((uint64_t) & v6);
    return v6;
}

/*
    TODO: Implement Loading/Saving from json configuration file, located beside this .asi file.
*/

/*
    Third-person aim sensitivity function:
    Supports setting the free-aim sensitivity, as well as the aiming sensitivity.
*/
__int64 __fastcall ThirdPersonMouseSensitivity(int64_t a1, int64_t a2, float a3, float a4)
{

    GameData::float_probe = {};
    GameData::byte_probe = {};
    GameData::dword_probe = {};
    GameData::qword_probe = {};

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
    float aiming_scale = 1 + aiming*(GameData::config.third_person_aim_scale - 1);
    float scale = GameData::config.third_person_sensitivity * aiming_scale * GameData::third_person_quake_scale;

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
    //1.82270
    GameData::float_probe = {};
    GameData::byte_probe = {};
    GameData::dword_probe = {};
    GameData::qword_probe = {};

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
    //v9 = Target::sub_3ADF20(a1, (*(BYTE*)(a2 + 211) & 8) != 0);

    GameData::float_probe.insert({ "v9", v9});

    // ADS check value. 0x8 -> ADS
    GameData::byte_probe.insert({ "*(BYTE*)(a2 + 211)", *(BYTE*)(a2 + 211)});

    GameData::float_probe.insert({ "(a1 + 88)", *(float*)(a1 + 88) });
    GameData::float_probe.insert({ "(a1 + 84)", *(float*)(a1 + 84) });

    GameData::float_probe.insert({ "(a1 + 80)", *(float*)(a1 + 80) });
    GameData::float_probe.insert({ "(a1 + 76)", *(float*)(a1 + 76) });
    GameData::float_probe.insert({ "(a1 + 48)", *(float*)(a1 + 48) });

    GameData::float_probe.insert({ "* (float*)Target::dword_39806FC", *(float*)Target::dword_39806FC });

    float aiming = 1.0f ? (*(BYTE*)(a2 + 211) & 8) == 0x8 : 0.0f; 
    float aiming_scale = 1 + aiming*(GameData::config.first_person_aim_scale- 1);
    // fov_scale applies a scaling to 'undo' the games scaling of the rotation it does some where else!
    float fov_scale = !GameData::config.scale_first_person_sensitivity_with_fov ? GameData::base_first_person_fov / *reinterpret_cast<float*>(GameData::fov_address) : 1.0f;
    float scale = GameData::config.first_person_sensitivity * aiming_scale * fov_scale * GameData::first_person_quake_scale;

    // This fixes the scaling to be the consistent across the non-scoped and scoped modes.
    *(float*)(a1 + 84) = 190.0f; 
    *(float*)(a1 + 76) = 250.0f; 

    v10 = (float)((float)((float)((float)(*(float*)(a1 + 88) - *(float*)(a1 + 84)) * v9) + *(float*)(a1 + 84)) * 0.017453292) * *(float*)Target::dword_39806FC * scale;
    a5 = (float)((float)((float)((float)(*(float*)(a1 + 80) - *(float*)(a1 + 76)) * v9) + *(float*)(a1 + 76)) * 0.017453292) * *(float*)Target::dword_39806FC * scale;
    v13 = v10;
    
    //Target::sub_391B78(&a5, &v13);
    
    v11 = -3.1415927;
    *(float*)(a2 + 128) = (float)(a4 * *(float*)(a1 + 240)) * a5;
    
    v12 = (float)(v8 * *(float*)(a1 + 244)) * v13;
    
    if (v12 >= -3.1415927)
        v11 = fminf(v12, 3.1415927);
    *(float*)(a2 + 136) = v11;

    //if (a4 != 0.0f)
    //    *(float*)(a2 + 128) = 0.05f * a4 / abs(a4);
    //if (v8 != 0.0f)
    //    *(float*)(a2 + 136) = 0.05f * v8 / abs(v8);

    GameData::float_probe.insert({ "(a2 + 128)", *(float*)(a2 + 128) });
    GameData::float_probe.insert({ "(a2 + 136)", *(float*)(a2 + 136) });
    GameData::float_probe.insert({ "(a1 + 240)", *(float*)(a1 + 240) });
    GameData::float_probe.insert({ "(a1 + 244)", *(float*)(a1 + 244) });
    GameData::float_probe.insert({ "*(float*)(a1 + 172)", *(float*)(a1 + 172) });
    GameData::float_probe.insert({ "*(float*)(a1 + 168)", *(float*)(a1 + 168) });
    
    GameData::dword_probe.insert({ "*(DWORD*)(a1 + 228)", *(DWORD*)(a1 + 228) });
}

void HookGameFunctions()
{
    GameData::baseAddress = (uintptr_t)GetModuleHandleA(NULL);
    LOG_DEBUG("Base address: 0x%llx", GameData::baseAddress);

    Target::sub_4162B0 = reinterpret_cast<Target::t_sub_4162B0>(GameData::baseAddress + 0x4162B0);
    Target::sub_2D32BE0 = reinterpret_cast<Target::t_sub_2D32BE0>(GameData::baseAddress + 0x2D32BE0);
    Target::sub_263DAF4 = reinterpret_cast<Target::t_sub_263DAF4>(GameData::baseAddress + 0x263DAF4);
    Target::sub_3ADEA8 = reinterpret_cast<Target::t_sub_3ADEA8>(GameData::baseAddress + 0x3ADEA8);
    Target::sub_391B78 = reinterpret_cast<Target::t_sub_391B78>(GameData::baseAddress + 0x391B78);
    Target::sub_3915A8 = reinterpret_cast<Target::t_sub_3915A8>(GameData::baseAddress + 0x3915A8);
    Target::sub_3923E4 = reinterpret_cast<Target::t_sub_3923E4>(GameData::baseAddress + 0x3923E4);
    Target::sub_3ADF20 = reinterpret_cast<Target::t_sub_3ADF20>(GameData::baseAddress + 0x3ADF20);
    Target::sub_173550 = reinterpret_cast<Target::t_sub_173550>(GameData::baseAddress + 0x173550);


    Target::sub_1B5C09C = reinterpret_cast<Target::t_sub_1B5C09C>(GameData::baseAddress + 0x1B5C09C);
    Target::sub_112F1D8 = reinterpret_cast<Target::t_sub_112F1D8>(GameData::baseAddress + 0x112F1D8);
    Target::sub_1489360 = reinterpret_cast<Target::t_sub_1489360>(GameData::baseAddress + 0x1489360);
    Target::sub_2C3F13C = reinterpret_cast<Target::t_sub_2C3F13C>(GameData::baseAddress + 0x2C3F13C);
    Target::sub_11A5EBC = reinterpret_cast<Target::t_sub_11A5EBC>(GameData::baseAddress + 0x11A5EBC);
    Target::sub_2C3F0FC = reinterpret_cast<Target::t_sub_2C3F0FC>(GameData::baseAddress + 0x2C3F0FC);
    Target::sub_3B83FC = reinterpret_cast<Target::t_sub_3B83FC>(GameData::baseAddress + 0x3B83FC);



    Target::qword_571A380 = (int64_t*)(GameData::baseAddress + 0x571A380);
    Target::dword_39806BC = (int64_t*)(GameData::baseAddress + 0x39806BC);
    Target::dword_39806FC = (int64_t*)(GameData::baseAddress + 0x39806FC);
    Target::dword_3973128 = (int64_t*)(GameData::baseAddress + 0x3973128);
    Target::dword_39ECE40 = (int64_t*)(GameData::baseAddress + 0x39ECE40);
    Target::qword_39ECE48 = (int64_t*)(GameData::baseAddress + 0x39ECE48);
        
    GameData::thirdPersonHookAddress = GameData::baseAddress + 0x395E18;
    if(MH_CreateHook((LPVOID)GameData::thirdPersonHookAddress, &ThirdPersonMouseSensitivity, NULL) == MH_OK)
    {
        LOG_INFO("%s: ThirdPersonMouseSensitivity hook created successfully.", __func__);
    }
    else {
        LOG_ERROR("%s: ThirdPersonMouseSensitivity hook creation failed.", __func__);
        return;
    }
    if(MH_EnableHook((LPVOID)GameData::thirdPersonHookAddress) == MH_OK)
    {
        LOG_INFO("%s: ThirdPersonMouseSensitivity hook enabled.", __func__);
    }
    else {
        LOG_ERROR("%s: ThirdPersonMouseSensitivity hook enable failed", __func__);
        return;
    }
    GameData::firstPersonHookAddress = GameData::baseAddress + 0x395EE0;
    if(MH_CreateHook((LPVOID)GameData::firstPersonHookAddress, &FirstPersonMouseSensitivity, NULL) == MH_OK)
    {
        LOG_INFO("%s: FirstPersonMouseSensitivity hook created successfully.", __func__);
    }
    else {
        LOG_ERROR("%s: FirstPersonMouseSensitivity hook creation failed.", __func__);
        return;
    }
    if(MH_EnableHook((LPVOID)GameData::firstPersonHookAddress) == MH_OK)
    {
        LOG_INFO("%s: FirstPersonMouseSensitivity hook enabled.", __func__);
    }
    else {
        LOG_ERROR("%s: FirstPersonMouseSensitivity hook enable failed", __func__);
        return;
    }


    GameData::fov_address = GameData::baseAddress + 0x3EA0BE0;

    // Load config from file
    if(!Mod::LoadConfigFromFile(GameData::config, Mod::kConfigFileName))
    {
        Mod::LoadDefault(GameData::config);
        Mod::SaveConfigToFile(GameData::config, Mod::kConfigFileName);
    }


    // Print the loaded configuration:
    {
        nlohmann::json json_print;
        Mod::to_json(json_print, GameData::config);
        std::string debug_json_print = json_print.dump();
        LOG_INFO("Loaded Config: %s", debug_json_print.c_str());
    }

    GameData::show_menu = GameData::config.show_menu_on_start_up;

}

void UnhookGameFunctions()
{
    MH_DisableHook((LPVOID)GameData::thirdPersonHookAddress);
    MH_RemoveHook((LPVOID)GameData::thirdPersonHookAddress);
    MH_DisableHook((LPVOID)GameData::firstPersonHookAddress);
    MH_RemoveHook((LPVOID)GameData::firstPersonHookAddress);
    LOG_DEBUG("Game functions unhooked.");
}

