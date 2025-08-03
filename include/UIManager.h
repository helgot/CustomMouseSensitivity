#pragma once
#include "ConfigManager.h"
#include "DebugData.h"

namespace CustomSensitivity
{
    class UIManager
    {
    public:
        UIManager(ConfigManager& configManager);

        // Main method to draw the entire UI, called by the graphics hook
        void Render();
        
        // Method to process user input, called by the graphics hook
        void HandleInput();

        // Provides access to the debug data for other components like GameHook
        DebugData& GetDebugData();

    private:
        // Private helper methods for drawing specific parts of the UI
        void DrawConfigManagementHelper();
        void DrawControlConfigHelper();
        void DrawDebugHelper();
        
        template<class T>
        void PrintMapHelper(const std::unordered_map<std::string_view, T>& map);

        ConfigManager& m_configManager;
        DebugData m_debugData;
        bool m_isMenuVisible = false;
    };
}