#pragma once

#include <string_view>

#include <json.hpp>

#include "logger.h"

namespace CustomSensitivity
{
    constexpr std::string_view kConfigFileName = "CustomSensitivity.json";
    struct Config
    {
        float first_person_sensitivity = 1.0f;
        float first_person_aim_scale = 1.0f;
        float third_person_sensitivity = 1.0f;
        float third_person_aim_scale = 1.0f;
        bool scale_first_person_sensitivity_with_fov = true;
        bool show_menu_on_start_up = true;
        std::string log_level = "L_INFO"; 
        bool LoadConfigFromFile(std::string_view filepath);
        bool SaveConfigToFile(std::string_view filepath);
        void LoadDefault();
    };
    void to_json(nlohmann::json& j, const Config& config);
    void from_json(const nlohmann::json& j, Config& config);
}
