#pragma once
#include "IRenderHook.h"

#include <Windows.h>
#include <string>
#include <vector>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>

class RenderHookDX12 : public IRenderHook
{
public:
    // Interface methods:
    RenderHookDX12();
    ~RenderHookDX12();
    void Shutdown() override;
    void OnPresent() override;
    bool HookGraphicsAPI() override;
    void UnhookGraphicsAPI() override;
    void SetGUICallback(GuiRenderCallback callback) override;
    void SetHandleInputCallback(HandleInputCallback callback) override;
    std::string_view GetHookName() const override 
    {
        return "RenderHookDX12"; 
    }

private:
    // ImGui initialization and cleanup:
    void InitializeImGui(IDXGISwapChain3* pSwapChain);
    void DeinitializeImGui();

    // DX12 resource management
    void CreateRenderTarget();
    void CleanupRenderTarget();

    // Hooked DX12/DXGI functions:
    static HRESULT WINAPI Hooked_Present(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags);
    static HRESULT WINAPI Hooked_ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
    static void WINAPI Hooked_ExecuteCommandLists(ID3D12CommandQueue* pQueue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists);
    static HRESULT WINAPI Hooked_D3D12CreateDevice(IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, REFIID riid, void** ppDevice);

    // Hooked WndProc function:
    static LRESULT CALLBACK Hooked_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    // Original function pointers:
    using D3D12CreateDevice_t = HRESULT(WINAPI*)(IUnknown*, D3D_FEATURE_LEVEL, REFIID, void**);
    D3D12CreateDevice_t m_original_D3D12CreateDevice = nullptr;

    using Present_t = HRESULT(WINAPI*)(IDXGISwapChain3*, UINT, UINT);
    Present_t m_original_Present = nullptr;

    using ResizeBuffers_t = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
    ResizeBuffers_t m_original_ResizeBuffers = nullptr;

    using ExecuteCommandLists_t = void(WINAPI*)(ID3D12CommandQueue*, UINT, ID3D12CommandList* const*);
    ExecuteCommandLists_t m_original_ExecuteCommandLists = nullptr;

    // DX12 objects captured from the game or created for ImGui
    ID3D12Device* m_d3d12Device = nullptr;
    ID3D12CommandQueue* m_commandQueue = nullptr;
    IDXGISwapChain3* m_swapChain = nullptr;

    ID3D12DescriptorHeap* m_rtvDescriptorHeap = nullptr; // For Render Target Views
    ID3D12DescriptorHeap* m_srvDescriptorHeap = nullptr; // For Shader Resource Views (ImGui font)
    std::vector<ID3D12Resource*> m_mainRenderTargetResource;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_mainRenderTargetDescriptor;
    
    // Per-frame resources for command recording
    std::vector<ID3D12CommandAllocator*> m_commandAllocators;
    UINT m_numBackBuffers = 0;

    // Synchronization objects
    ID3D12Fence* m_fence = nullptr;
    UINT64 m_fenceValue = 0;
    HANDLE m_fenceEvent = nullptr;

    bool m_imguiInitialized = false;

    GuiRenderCallback m_guiCallback;
    HandleInputCallback m_handleInputCallback;

    // Win32 Objects:
    HWND m_hWnd = nullptr;
    WNDPROC m_original_WndProc = nullptr;

    static RenderHookDX12* s_instance;
};