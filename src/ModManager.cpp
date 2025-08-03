#include "ModManager.h"
#include "Logger.h"
#include <MinHook.h>

namespace CustomSensitivity
{
    ModManager& ModManager::GetInstance()
    {
        static ModManager instance;
        return instance;
    }

    ModManager::ModManager()
    {
        LOG_DEBUG("ModManager instance created.");
        // Create components in order of dependency
        m_configManager = std::make_unique<ConfigManager>();
        m_uiManager = std::make_unique<UIManager>(*m_configManager);
        m_gameHook = std::make_unique<GameHook>(m_configManager->m_config, m_uiManager->GetDebugData());
        m_graphicsHookManager = std::make_unique<GraphicsHookManager>();
    }

    ModManager::~ModManager()
    {
        LOG_DEBUG("ModManager instance destroyed.");
    }

    void ModManager::Start()
    {
        LOG_DEBUG("ModManager::Start() called");
        if (MH_Initialize() != MH_OK)
        {
            LOG_ERROR("Failed to initialize MinHook.");
            return;
        }

        // Hook game logic first
        if (!m_gameHook->Install())
        {
            LOG_ERROR("Failed to install game function hooks.");
            MH_Uninitialize();
            return;
        }

        // Set up the graphics hooks and callbacks
        m_graphicsHookManager->SetGuiRenderCallback([this]() { m_uiManager->Render(); });
        m_graphicsHookManager->SetHandleInputCallback([this]() { m_uiManager->HandleInput(); });
        m_graphicsHookManager->InstallHooks();
    }

    void ModManager::Shutdown()
    {
        LOG_DEBUG("ModManager::Shutdown() called");
        if (m_graphicsHookManager)
            m_graphicsHookManager->Shutdown();
            
        if (m_gameHook)
            m_gameHook->Uninstall();

        if (MH_Uninitialize() != MH_OK)
        {
            LOG_ERROR("Failed to uninitialize MinHook.");
        }
        else
        {
            LOG_DEBUG("MinHook uninitialized.");
        }
    }
}