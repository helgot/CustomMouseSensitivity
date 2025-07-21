#pragma once
#include <functional>

using GuiRenderCallback = std::function<void()>;

class IRenderHook
{
public:
	virtual void Shutdown() = 0;
	virtual void OnPresent() = 0;
	virtual void SetGUICallback(GuiRenderCallback callback) = 0;
	virtual void HookGraphicsAPI() = 0;
	virtual void UnhookGraphicsAPI() = 0;
};