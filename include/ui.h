#pragma once

#include <unordered_map>
#include <string_view>

namespace CustomSensitivity
{
    /* Forward class definitions that we'll need. */
    class Config;
    class DebugData;

    /* A collection of UI helper function for drawing the IMGUI UI. */
    template<class T>
    void ImGuiPrintMapHelper(const std::unordered_map<std::string_view, T>& map);
    void ImGuiDrawControlConfigHelper(Config& config);
    void ImGuiDrawConfigManagementHelper(Config& config);
    void ImGuiDrawDebugHelper(DebugData& debug_data);
}
