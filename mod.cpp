#include "Mod.h"

#include <MinHook.h>
#include "Logger.h"
#include "Script.h"
#include "renderhook_vulkan.h"

Mod& Mod::GetInstance()
{
    static Mod instance;
    LOG_DEBUG("Mod instance accessed.");
    return instance;
}

Mod::Mod()
{
    Logger::getInstance().setLevel(LogLevel::L_DEBUG);
    Logger::getInstance().setLogFile("CustomMouseSensivitity.log");
    LOG_DEBUG("Mod instance created.");

    m_rendererHook = std::make_unique<RenderHookVulkan>();
}

Mod::~Mod()
{
    Shutdown();
    LOG_DEBUG("Mod instance destroyed.");
}

void Mod::Start()
{
    if (!m_rendererHook) return;

    m_rendererHook->SetGUICallback(ImGuiDrawCommandsCallback);

    if (MH_Initialize() != MH_OK)
    {
        LOG_ERROR("Failed to initialize MinHook.");
        return;
    }
    LOG_DEBUG("MinHook initialized successfully.");

    m_rendererHook->HookGraphicsAPI();
    HookGameFunctions();
}

void Mod::Shutdown()
{
    if (m_rendererHook)
    {
        m_rendererHook->UnhookGraphicsAPI();
    }
    UnhookGameFunctions();

    if (MH_Uninitialize() != MH_OK)
    {
        LOG_ERROR("Failed to uninitialize MinHook.");
    }
    else
    {
        LOG_DEBUG("MinHook uninitialized.");
    }

    // The unique_ptr will automatically delete the renderer hook object.
    // No need for 'delete m_rendererHook'.
}