#pragma once
#include "i_renderhook.h"


class RenderHookD3D12 : public IRenderHook
{
public: 
	RenderHookD3D12();
	~RenderHookD3D12();
	void Shutdown() override;
	void OnPresent() override;
	bool HookGraphicsAPI() override;
	void UnhookGraphicsAPI() override;
	void SetGUICallback(GuiRenderCallback callback) override;
	void SetHandleInputCallback(HandleInputCallback callback) override;
private:

};