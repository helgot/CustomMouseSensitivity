#include "ui.h"


#include "imgui.h"

#include "debug_data.h"
#include "config.h"

namespace CustomSensitivity
{
    constexpr float kMinSliderValue = 0.0f;
    constexpr float kMaxSliderValue = 10.0f;
    constexpr float kDefaultValue = 1.0f; 

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

    void ImGuiDrawControlConfigHelper(Config& config)
    {
        ImGui::SliderFloat("First-Person Sensitivity", &config.first_person_sensitivity, kMinSliderValue, kMaxSliderValue);
        ImGui::SliderFloat("First-Person Aim Scale", &config.first_person_aim_scale, kMinSliderValue, kMaxSliderValue);
        ImGui::SliderFloat("Third-Person Sensitivity", &config.third_person_sensitivity, kMinSliderValue, kMaxSliderValue);
        ImGui::SliderFloat("Third-Person Aim Scale", &config.third_person_aim_scale, kMinSliderValue, kMaxSliderValue);
        ImGui::Checkbox("Scale First-Person Sensitivity With FOV", &config.scale_first_person_sensitivity_with_fov);
        ImGui::Checkbox("Show Menu On Start-up ", &config.show_menu_on_start_up);
    }

    void ImGuiDrawConfigManagementHelper(Config& config)
    {
        ImGui::Separator();
        if (ImGui::Button("Save Config"))
        {
            config.SaveConfigToFile(kConfigFileName);
        }

        ImGui::SameLine();

        if (ImGui::Button("Load Config"))
        {
            config.LoadConfigFromFile(kConfigFileName);
        }

        ImGui::SameLine();

        if (ImGui::Button("Set Default"))
        {
            config.LoadDefault();
        }
    }

    void ImGuiDrawDebugHelper(DebugData& debug_data)
    {
        ImGui::Text("FOV: %0.2f", *reinterpret_cast<float*>(debug_data.fov_address));
        ImGui::Separator();
        ImGui::Text("Debug Probes:");
        ImGui::Separator();
        ImGuiPrintMapHelper(debug_data.float_probe);
        ImGui::Separator();
        ImGuiPrintMapHelper(debug_data.byte_probe);
        ImGui::Separator();
        ImGuiPrintMapHelper(debug_data.dword_probe);
        ImGui::Separator();
        ImGuiPrintMapHelper(debug_data.qword_probe);
        ImGui::Separator();
        ImGuiPrintMapHelper(debug_data.int_probe);
        ImGui::Separator();
    }

}