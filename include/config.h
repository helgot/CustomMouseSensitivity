#pragma once

#include <fstream>
#include <json.hpp>
#include <string>
#include <string_view>

namespace CustomSensitivity
{
constexpr std::string_view kConfigFileName = "CustomSensitivity.json";

struct Config
{
    float first_person_sensitivity = 1.0f;
    float first_person_aim_scale = 1.0f;
    float third_person_sensitivity = 1.0f;
    float third_person_aim_scale = 1.0f;
    bool undo_first_person_fov_scaling = false;
    bool show_menu_on_start_up = true;
    std::string log_level = "L_INFO";
};

void to_json(nlohmann::json &j, const Config &config);
void from_json(const nlohmann::json &j, Config &config);
} // namespace CustomSensitivity