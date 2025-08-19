#pragma once
#include "Config.h" // Include the existing Config struct definition

namespace CustomSensitivity
{
class ConfigManager
{
public:
    ConfigManager();
    bool SaveConfig();
    void LoadConfig();

    Config m_config;
};
} // namespace CustomSensitivity