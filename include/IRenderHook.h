#pragma once
#include <functional>
#include <string_view>

using GuiRenderCallback = std::function<void()>;
using HandleInputCallback = std::function<void()>;

class IRenderHook
{
public:
    virtual void Shutdown() = 0;
    virtual void OnPresent() = 0;
    virtual void SetGUICallback(GuiRenderCallback callback) = 0;
    virtual void SetHandleInputCallback(HandleInputCallback callback) = 0;
    virtual bool HookGraphicsAPI() = 0;
    virtual void UnhookGraphicsAPI() = 0;
    virtual std::string_view GetHookName() const = 0;
};