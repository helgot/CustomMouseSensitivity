#include "ConfigManager.h"
#include <fstream>
#include <string>

#include "Config.h"
#include "Logger.h"

namespace CustomSensitivity
{
    // The to_json/from_json functions for the Config struct remain the same
    // as you had before, as they are essential for the nlohmann::json library.
    void to_json(nlohmann::json& j, const Config& config)
    {
        j = {
            { "first_person_sensitivity", config.first_person_sensitivity },
            { "first_person_aim_scale", config.first_person_aim_scale },
            { "third_person_sensitivity", config.third_person_sensitivity },
            { "third_person_aim_scale", config.third_person_aim_scale },
            { "scale_first_person_sensitivity_with_fov", config.scale_first_person_sensitivity_with_fov },
            { "show_menu_on_start_up", config.show_menu_on_start_up },
            { "log_level", config.log_level }
        };
    }

    void from_json(const nlohmann::json& j, Config& config)
    {
        // Using .value() is safer than .at() as it provides a default if the key is missing
        config.first_person_sensitivity = j.value("first_person_sensitivity", 1.0f);
        config.first_person_aim_scale = j.value("first_person_aim_scale", 1.0f);
        config.third_person_sensitivity = j.value("third_person_sensitivity", 1.0f);
        config.third_person_aim_scale = j.value("third_person_aim_scale", 1.0f);
        config.scale_first_person_sensitivity_with_fov = j.value("scale_first_person_sensitivity_with_fov", true);
        config.show_menu_on_start_up = j.value("show_menu_on_start_up", true);
        config.log_level = j.value("log_level", "L_INFO");
    }
    
    // --- ConfigManager Implementation ---

    ConfigManager::ConfigManager()
    {
        LoadConfig();
    }

    bool ConfigManager::SaveConfig()
    {
        try {
            std::ofstream file(kConfigFileName.data());
            if (!file) {
                LOG_ERROR("Failed to open %s for writing.", kConfigFileName.data());
                return false;
            }

            nlohmann::json j = m_config;
            file << j.dump(4); // pretty print with 4 spaces
            LOG_INFO("Configuration saved to %s", kConfigFileName.data());
            return true;
        }
        catch (const std::exception& e) {
            LOG_ERROR("Exception while saving config: %s", e.what());
            return false;
        }
    }

    void ConfigManager::LoadConfig()
    {
        std::ifstream file(kConfigFileName.data());
        if (!file.is_open())
        {
            LOG_INFO("%s not found. Creating default configuration.", kConfigFileName.data());
            m_config = {}; // Set to default values
            SaveConfig(); // Save the new default config file
            return;
        }

        try {
            nlohmann::json j;
            file >> j;
            m_config = j.get<Config>();
            LOG_INFO("Configuration loaded from %s", kConfigFileName.data());
        }
        catch (const std::exception& e) {
            LOG_ERROR("Failed to parse %s: %s. Loading default config.", kConfigFileName.data(), e.what());
            m_config = {}; // Load defaults if the file is corrupt
        }
    }
}