#include "config.h"


namespace CustomSensitivity
{
    void to_json(nlohmann::json& j, const Config& config)
    {
        j = {
            { "first_person_sensitivity", config.first_person_sensitivity },
            { "first_person_aim_scale", config.first_person_aim_scale },
            { "third_person_sensitivity", config.third_person_sensitivity },
            { "third_person_aim_scale", config.third_person_aim_scale },
            { "scale_first_person_sensitivity_with_fov", config.scale_first_person_sensitivity_with_fov },
            { "show_menu_on_start_up", config.show_menu_on_start_up},
            { "log_level", config.log_level}

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
        j.at("log_level").get_to(config.log_level);
    }

    bool Config::SaveConfigToFile(std::string_view filepath)
    {
        try {
            std::ofstream file(std::string(filepath).c_str());
            if (!file) return false;
            nlohmann::json j = *this;
            file << j.dump(4);
            return true;
        } catch (const std::exception& e){
            LOG_DEBUG(e.what());
            return false;
        }
    }
    
    void Config::LoadDefault()
    {
        *this = {};
    }
    
    bool Config::LoadConfigFromFile(std::string_view filepath)
    {
        try {
            std::ifstream file(std::string(filepath).c_str());
            if (!file) 
            {
                LoadDefault();
                SaveConfigToFile(filepath);
                return false;
            }
            nlohmann::json j;
            file >> j;
            *this = j.get<Config>();
            return true;
        } catch (const std::exception& e){
            LOG_DEBUG(e.what());
            LoadDefault();
            return false;
        }
    }

}