#include "GraphicsHookManager.h"

#include <ShlObj.h> // Required for SHGetKnownFolderPath
#include <fstream>
#include <sstream>
#include <string>

#include "logger.h"

#include "RenderHookDX12.h"
#include "RenderHookVulkan.h"

// Helper function to read a file into a string
std::string ReadFileToString(const std::wstring &filepath)
{
    std::ifstream file_stream(filepath);
    if (!file_stream.is_open())
    {
        return "";
    }
    std::stringstream buffer;
    buffer << file_stream.rdbuf();
    return buffer.str();
}

void GraphicsHookManager::InstallHooks()
{
    LOG_DEBUG("HookManager: Determining active graphics API from settings...");

    GraphicsApi configured_api = GraphicsApi::Unknown;
    PWSTR documents_path = NULL;

    // Programmatically get the path to the user's Documents folder
    if (SUCCEEDED(
            SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &documents_path)))
    {
        std::wstring settings_path = documents_path;
        settings_path +=
            L"\\Rockstar Games\\Red Dead Redemption 2\\Settings\\system.xml";
        CoTaskMemFree(
            documents_path); // Free the memory allocated by the system

        LOG_DEBUG("HookManager: Reading settings file at %ws",
                  settings_path.c_str());
        std::string settings_content = ReadFileToString(settings_path);

        if (settings_content.find("kSettingAPI_Vulkan") != std::string::npos)
        {
            configured_api = GraphicsApi::Vulkan;
            LOG_DEBUG("HookManager: Vulkan API is configured in settings.");
        }
        else if (settings_content.find("kSettingAPI_DX12") != std::string::npos)
        {
            configured_api = GraphicsApi::DX12;
            LOG_DEBUG("HookManager: DirectX 12 API is configured in settings.");
        }
        else
        {
            LOG_DEBUG(
                "HookManager: Could not determine API from settings file.");
            return;
        }
    }
    else
    {
        LOG_DEBUG("HookManager: Could not find user Documents folder.");
        return;
    }

    // Now, wait for the correct DLL to load and install ONLY the necessary
    // hooks.
    if (configured_api == GraphicsApi::Vulkan)
    {
        while (GetModuleHandleA("vulkan-1.dll") == nullptr)
        {
            Sleep(100);
        }
        LOG_DEBUG(
            "HookManager: Vulkan module loaded. Initializing Vulkan hooks...");
        m_activeApi = GraphicsApi::Vulkan;
        m_renderHook = std::make_unique<RenderHookVulkan>();
        m_renderHook->HookGraphicsAPI();
        m_renderHook->SetGUICallback(m_guiCallback);
        m_renderHook->SetHandleInputCallback(m_handleInputCallback);
    }
    else if (configured_api == GraphicsApi::DX12)
    {
        while (GetModuleHandleA("d3d12.dll") == nullptr)
        {
            Sleep(100);
        }
        LOG_DEBUG(
            "HookManager: D3D12 module loaded. Initializing DX12 hooks...");
        m_activeApi = GraphicsApi::DX12;
        m_renderHook = std::make_unique<RenderHookDX12>();
        m_renderHook->HookGraphicsAPI();
        m_renderHook->SetGUICallback(m_guiCallback);
        m_renderHook->SetHandleInputCallback(m_handleInputCallback);
    }
}

void GraphicsHookManager::Shutdown()
{
    if (m_renderHook)
    {
        m_renderHook->Shutdown();
        m_renderHook.reset(); // Release the unique_ptr
    }
}

void GraphicsHookManager::SetGuiRenderCallback(GuiRenderCallback callback)
{
    m_guiCallback = std::move(callback);
}

void GraphicsHookManager::SetHandleInputCallback(HandleInputCallback callback)
{
    m_handleInputCallback = std::move(callback);
}