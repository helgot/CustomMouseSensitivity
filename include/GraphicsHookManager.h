#pragma once
#include <memory>
#include <functional>

#include <d3d12.h>
#include <vulkan/vulkan.h>
#include <dxgi1_4.h>

#include "IRenderHook.h"

enum class GraphicsApi
{
    Unknown,
    DX12,
    Vulkan
};

class GraphicsHookManager
{
public:
    GraphicsHookManager() = default;// Public constructor
    ~GraphicsHookManager() = default; // Public destructor
    void InstallHooks();
    void Shutdown();

    using GuiRenderCallback = std::function<void()>;
    using HandleInputCallback = std::function<void()>;

    void SetGuiRenderCallback(GuiRenderCallback callback);
    void SetHandleInputCallback(HandleInputCallback callback);

    // Prevent copying
    GraphicsHookManager(const GraphicsHookManager&) = delete;
    void operator=(const GraphicsHookManager&) = delete;

private:
    // The active render hook (either DX12 or Vulkan)
    std::unique_ptr<IRenderHook> m_renderHook = nullptr;
    GraphicsApi m_activeApi = GraphicsApi::Unknown;

    GuiRenderCallback m_guiCallback;
    HandleInputCallback m_handleInputCallback;
};