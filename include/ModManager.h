#pragma once

#include <memory>
#include "GraphicsHookManager.h"
#include "GameHook.h"
#include "UIManager.h"
#include "ConfigManager.h"

namespace CustomSensitivity
{
    class ModManager
    {
    public:
        static ModManager& GetInstance();
        void Start();
        void Shutdown();

        // Deleted constructors for singleton pattern
        ModManager(const ModManager&) = delete;
        void operator=(const ModManager&) =delete;

    private:
        ModManager();
        ~ModManager();

        std::unique_ptr<GraphicsHookManager> m_graphicsHookManager;
        std::unique_ptr<GameHook> m_gameHook;
        std::unique_ptr<UIManager> m_uiManager;
        std::unique_ptr<ConfigManager> m_configManager;
    };
}