#pragma once

using GuiRenderCallback = void(*)();
using HandleInputCallback = void(*)();

class IRenderHook
{
public:
	virtual void Shutdown() = 0;
	virtual void OnPresent() = 0;
	virtual void SetGUICallback(GuiRenderCallback callback) = 0;
	virtual void SetHandleInputCallback(HandleInputCallback callback) = 0;
	virtual bool HookGraphicsAPI() = 0;
	virtual void UnhookGraphicsAPI() = 0;
};