#pragma once

#include <fstream>
#include <json.hpp>
#include <string>
#include <string_view>

namespace CustomSensitivity
{
constexpr std::string_view kConfigFileName = "CustomSensitivity.json";

// This struct purely holds the configuration data.
struct Config
{
    float first_person_sensitivity = 1.0f;
    float first_person_aim_scale = 1.0f;
    float third_person_sensitivity = 1.0f;
    float third_person_aim_scale = 1.0f;
    bool scale_first_person_sensitivity_with_fov = true;
    bool show_menu_on_start_up = true;
    std::string log_level = "L_INFO";
};

// Forward declarations for JSON serialization functions.
// The implementation is now in ConfigManager.cpp.
void to_json(nlohmann::json &j, const Config &config);
void from_json(const nlohmann::json &j, Config &config);
} // namespace CustomSensitivity