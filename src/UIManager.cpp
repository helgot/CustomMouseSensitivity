#include "UIManager.h"
#include <Windows.h>
#include "imgui.h"

namespace CustomSensitivity
{
    UIManager::UIManager(ConfigManager& configManager)
        : m_configManager(configManager)
    {
        // Initialize the menu visibility from the loaded config
        m_isMenuVisible = m_configManager.m_config.show_menu_on_start_up;
    }

    DebugData& UIManager::GetDebugData()
    {
        return m_debugData;
    }

    void UIManager::Render()
    {
        if (!m_isMenuVisible)
            return;

        ImGui::Begin("Custom Sensitivity (Toggle: F10)");
        DrawConfigManagementHelper();

        if (ImGui::BeginTabBar("MainTabBar"))
        {
            if (ImGui::BeginTabItem("Config"))
            {
                DrawControlConfigHelper();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Debug"))
            {
                DrawDebugHelper();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    }

    void UIManager::HandleInput()
    {
        static bool wasF10Pressed = false;
        bool isF10Pressed = GetAsyncKeyState(VK_F10) & 0x8000;

        if (isF10Pressed && !wasF10Pressed)
        {
            m_isMenuVisible = !m_isMenuVisible;

            // Show/hide the mouse cursor along with the menu
            ImGuiIO& io = ImGui::GetIO();
            io.MouseDrawCursor = m_isMenuVisible;
        }
        wasF10Pressed = isF10Pressed;
    }

    // --- Private Helper Methods ---
    void UIManager::DrawControlConfigHelper()
    {
        constexpr float min_val = 0.0f;
        constexpr float max_val = 10.0f;

        ImGui::SliderFloat("First-Person Sensitivity", &m_configManager.m_config.first_person_sensitivity, min_val, max_val);
        ImGui::SliderFloat("First-Person Aim Scale", &m_configManager.m_config.first_person_aim_scale, min_val, max_val);
        ImGui::SliderFloat("Third-Person Sensitivity", &m_configManager.m_config.third_person_sensitivity, min_val, max_val);
        ImGui::SliderFloat("Third-Person Aim Scale", &m_configManager.m_config.third_person_aim_scale, min_val, max_val);
        ImGui::Checkbox("Scale First-Person Sensitivity With FOV", &m_configManager.m_config.scale_first_person_sensitivity_with_fov);
        ImGui::Checkbox("Show Menu on Start-up:", &m_configManager.m_config.show_menu_on_start_up);
    }

    void UIManager::DrawConfigManagementHelper()
    {
        ImGui::Separator();
        if (ImGui::Button("Save Config"))
        {
            // Update the main config and save
            m_configManager.SaveConfig();
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Config"))
        {
            // Reload from disk into the manager, then update our editable copy
            m_configManager.LoadConfig();
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Defaults"))
        {
            // Create a default config and update our editable copy
            m_configManager.m_config = Config{}; 
        }
        ImGui::Separator();
    }

    void UIManager::DrawDebugHelper()
    {
        if (m_debugData.fov_address)
            ImGui::Text("FOV: %.2f", *reinterpret_cast<float*>(m_debugData.fov_address));
        else
            ImGui::Text("FOV: (Address not set)");

        ImGui::Separator();
        ImGui::Text("Debug Probes:");
        PrintMapHelper(m_debugData.float_probe);
        PrintMapHelper(m_debugData.byte_probe);
        PrintMapHelper(m_debugData.dword_probe);
        PrintMapHelper(m_debugData.qword_probe);
        PrintMapHelper(m_debugData.int_probe);
    }

    template<class T>
    void UIManager::PrintMapHelper(const std::unordered_map<std::string_view, T>& map)
    {
        if (map.empty()) return;

        ImGui::SeparatorText(typeid(T).name()); // Display type name as a header
        for (const auto& [key, value] : map)
        {
            if constexpr (std::is_same_v<T, int>)
                ImGui::Text("  %s: %d", key.data(), value);
            else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>)
                ImGui::Text("  %s: %.3f", key.data(), static_cast<double>(value));
            else if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, BYTE>)
                ImGui::Text("  %s: 0x%02X", key.data(), value);
            else if constexpr (std::is_same_v<T, uint32_t>)
                ImGui::Text("  %s: 0x%08X", key.data(), value);
            else if constexpr (std::is_same_v<T, uint64_t>)
                ImGui::Text("  %s: 0x%016llX", key.data(), value);
            else
                ImGui::Text("  %s: [unsupported type]", key.data());
        }
    }
}