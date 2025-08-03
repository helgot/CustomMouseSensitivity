#include "RenderHookDX12.h"

#include "logger.h"
#include <MinHook.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

// Forward declare the ImGui Win32 WndProc handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

template<typename T>
static inline void SafeRelease(T*& res)
{
    if (res)
        res->Release();
    res = nullptr;
}

RenderHookDX12* RenderHookDX12::s_instance = nullptr;

RenderHookDX12::RenderHookDX12()
{
    s_instance = this;
}

RenderHookDX12::~RenderHookDX12()
{
    Shutdown();
    s_instance = nullptr;
}

void RenderHookDX12::Shutdown()
{
    // Wait for the GPU to be done with all resources.
    if (m_commandQueue && m_fence && m_fenceEvent)
    {
        const UINT64 fence = m_fenceValue;
        m_commandQueue->Signal(m_fence, fence);
        m_fenceValue++;
        if (m_fence->GetCompletedValue() < fence)
        {
            m_fence->SetEventOnCompletion(fence, m_fenceEvent);
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }

    DeinitializeImGui();
    UnhookGraphicsAPI();
}

void RenderHookDX12::OnPresent()
{
    if (m_guiCallback)
    {
        m_guiCallback();
    }
    else
    {
        LOG_DEBUG("No GUI callback set, skipping ImGui rendering.");
    }
}

bool RenderHookDX12::HookGraphicsAPI()
{

    // DEBUG: For enabling DX12 DEBUG layers.
    //HMODULE hD3D12 = GetModuleHandleA("d3d12.dll");
    //if (hD3D12)
    //{
    //    void* pCreateDevice = GetProcAddress(hD3D12, "D3D12CreateDevice");
    //    if (MH_CreateHook(pCreateDevice, &Hooked_D3D12CreateDevice, reinterpret_cast<void**>(&m_original_D3D12CreateDevice)) != MH_OK ||
    //        MH_EnableHook(pCreateDevice) != MH_OK) {
    //        LOG_DEBUG("Failed to hook D3D12CreateDevice!");
    //        // This is critical, so we might want to fail here.
    //        return false;
    //    }
    //    LOG_DEBUG("Hook for D3D12CreateDevice enabled.");
    //}

    // To hook a v-table function, we need an instance of the class.
    // We create a dummy swap chain to get the v-table addresses.
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "DX12 Dummy Window", NULL };
    RegisterClassEx(&wc);
    HWND dummy_hwnd = CreateWindow(wc.lpszClassName, "DX12 Dummy", WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, NULL, NULL, wc.hInstance, NULL);

    ID3D12Device* pDevice = nullptr;
    ID3D12CommandQueue* pCommandQueue = nullptr;
    IDXGISwapChain* pTmpSwapChain = nullptr;
    IDXGISwapChain3* pSwapChain = nullptr;

    // Create device
    if (FAILED(D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDevice))))
    {
        DestroyWindow(dummy_hwnd);
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        LOG_DEBUG("Failed to create dummy D3D12 device.");
        return false;
    }

    // Create command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    if (FAILED(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCommandQueue))))
    {
        pDevice->Release();
        DestroyWindow(dummy_hwnd);
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        LOG_DEBUG("Failed to create dummy command queue.");
        return false;
    }

    // Create swap chain
    IDXGIFactory4* pFactory = nullptr;
    CreateDXGIFactory1(IID_PPV_ARGS(&pFactory));
    if (pFactory)
    {
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 2;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sd.OutputWindow = dummy_hwnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        
        if (SUCCEEDED(pFactory->CreateSwapChain(pCommandQueue, &sd, &pTmpSwapChain))) {
            pTmpSwapChain->QueryInterface(IID_PPV_ARGS(&pSwapChain));
            pTmpSwapChain->Release();
        }
        pFactory->Release();
    }

    if (!pSwapChain)
    {
        pCommandQueue->Release();
        pDevice->Release();
        DestroyWindow(dummy_hwnd);
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        LOG_DEBUG("Failed to create dummy swap chain.");
        return false;
    }

    // Get v-table addresses
    void** pVTable = *(void***)pSwapChain;
    void* m_target_Present = (Present_t)pVTable[8];
    void* m_target_ResizeBuffers = (ResizeBuffers_t)pVTable[13];


    void** pVTable_Queue = *(void***)pCommandQueue;
    void* m_target_ExecuteCommandLists = pVTable_Queue[10];

    LOG_DEBUG("%s: m_original_Present: 0x%llx",__func__, m_original_Present);
    LOG_DEBUG("%s: m_original_ResizeBuffers: 0x%llx",__func__, m_original_ResizeBuffers);


    // Cleanup dummy objects
    pSwapChain->Release();
    pCommandQueue->Release();
    pDevice->Release();
    DestroyWindow(dummy_hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    // Create and enable hooks
    if (MH_CreateHook(m_target_Present, &Hooked_Present, reinterpret_cast<void**>(&m_original_Present)) != MH_OK) {
        LOG_DEBUG("Failed to create hook for Present!");
        return false;
    }
    if (MH_EnableHook(m_target_Present) != MH_OK) {
        LOG_DEBUG("Failed to enable hook for Present!");
        return false;
    }
    LOG_DEBUG("Hook for Present enabled.");
    
    if (MH_CreateHook(m_target_ResizeBuffers, &Hooked_ResizeBuffers, reinterpret_cast<void**>(&m_original_ResizeBuffers)) != MH_OK) {
        LOG_DEBUG("Failed to create hook for ResizeBuffers!");
        return false;
    }
    if (MH_EnableHook(m_target_ResizeBuffers) != MH_OK) {
        LOG_DEBUG("Failed to enable hook for ResizeBuffers!");
        return false;
    }
    LOG_DEBUG("Hook for ResizeBuffers enabled.");

    // ExecuteCommandLists Hook
    if (MH_CreateHook(m_target_ExecuteCommandLists, &Hooked_ExecuteCommandLists, reinterpret_cast<void**>(&m_original_ExecuteCommandLists)) != MH_OK ||
        MH_EnableHook(m_target_ExecuteCommandLists) != MH_OK) {
        LOG_DEBUG("Failed to hook ExecuteCommandLists!");
        return false;
    }
    LOG_DEBUG("Hook for ExecuteCommandLists enabled.");

    LOG_DEBUG("%s: m_original_Present: 0x%llx",__func__, m_original_Present);
    LOG_DEBUG("%s: m_original_ResizeBuffers: 0x%llx",__func__, m_original_ResizeBuffers);
    LOG_DEBUG("%s: m_original_ExecuteCommandLists: 0x%llx",__func__, m_original_ExecuteCommandLists);

    LOG_DEBUG("DirectX 12 API hooks installed successfully.");
    return true;
}

void RenderHookDX12::UnhookGraphicsAPI()
{
    MH_DisableHook(m_original_Present);
    MH_DisableHook(m_original_ResizeBuffers);
    LOG_DEBUG("DirectX 12 Hooks Disabled.");
}

void RenderHookDX12::InitializeImGui(IDXGISwapChain3* pSwapChain)
{
    if (m_imguiInitialized) return;

    if (!m_d3d12Device || !m_commandQueue) {
        LOG_DEBUG("Device or Command Queue not captured. Cannot initialize ImGui.");
        return;
    }

    LOG_DEBUG("Initializing ImGui for DX12...");

    DXGI_SWAP_CHAIN_DESC desc;
    pSwapChain->GetDesc(&desc);
    m_numBackBuffers = desc.BufferCount;
    m_hWnd = desc.OutputWindow;

    // Create RTV descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
    rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv_heap_desc.NumDescriptors = m_numBackBuffers;
    rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(m_d3d12Device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&m_rtvDescriptorHeap)))) {
        LOG_DEBUG("FATAL: Failed to create RTV descriptor heap.");
        return; 
    }

    // Create SRV descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = {};
    srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srv_heap_desc.NumDescriptors = 1;
    srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    if (FAILED(m_d3d12Device->CreateDescriptorHeap(&srv_heap_desc, IID_PPV_ARGS(&m_srvDescriptorHeap)))) {
        LOG_DEBUG("FATAL: Failed to create SRV descriptor heap.");
        SafeRelease(m_rtvDescriptorHeap); // Clean up previously created heap
        return; 
    }

    m_commandAllocators.resize(m_numBackBuffers);
    for (UINT i = 0; i < m_numBackBuffers; i++) {
        if (FAILED(m_d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i])))) {
            LOG_DEBUG("FATAL: Failed to create command allocator for frame %u.", i);
            // Perform full cleanup before returning
            return;
        }
    }
    LOG_DEBUG("Created %u command allocators.", m_numBackBuffers);

    // Create render target views for swap chain buffers
    CreateRenderTarget();

    // Create synchronization objects
    if (FAILED(m_d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)))) {
         LOG_DEBUG("FATAL: Failed to create fence.");
         // Cleanup...
         return;
    }
    m_fenceValue = 1;
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);


    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    ImGui::StyleColorsDark();

    // Hook WndProc
    m_original_WndProc = (WNDPROC)SetWindowLongPtrA(m_hWnd, GWLP_WNDPROC, (LONG_PTR)RenderHookDX12::Hooked_WndProc);
    ImGui_ImplWin32_Init(m_hWnd);
    
    // Setup the modern InitInfo struct
    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = m_d3d12Device;
    init_info.NumFramesInFlight = m_numBackBuffers;
    init_info.RTVFormat = desc.BufferDesc.Format;
    init_info.CommandQueue = m_commandQueue;
    init_info.SrvDescriptorHeap = m_srvDescriptorHeap;
    init_info.LegacySingleSrvCpuDescriptor = m_srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    init_info.LegacySingleSrvGpuDescriptor = m_srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    
    // Call the modern Init function
    if (ImGui_ImplDX12_Init(&init_info)) {
        m_imguiInitialized = true;
        LOG_DEBUG("ImGui for DX12 initialized successfully.");
    } else {
        LOG_DEBUG("FATAL: ImGui_ImplDX12_Init failed.");
        ImGui_ImplWin32_Shutdown();
        SetWindowLongPtrA(m_hWnd, GWLP_WNDPROC, (LONG_PTR)m_original_WndProc);
        // ... more cleanup
    }
}
void RenderHookDX12::DeinitializeImGui()
{
    if (!m_imguiInitialized) return;

    LOG_DEBUG("Shutting down ImGui DX12 backend...");

    // Restore original WndProc
    if (m_original_WndProc) {
        SetWindowLongPtrA(m_hWnd, GWLP_WNDPROC, (LONG_PTR)m_original_WndProc);
        m_original_WndProc = nullptr;
    }

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupRenderTarget();

    if (m_srvDescriptorHeap) { m_srvDescriptorHeap->Release(); m_srvDescriptorHeap = nullptr; }
    if (m_rtvDescriptorHeap) { m_rtvDescriptorHeap->Release(); m_rtvDescriptorHeap = nullptr; }
    
    for (auto& allocator : m_commandAllocators) {
        if (allocator) allocator->Release();
    }
    m_commandAllocators.clear();

    if (m_commandQueue) { m_commandQueue->Release(); m_commandQueue = nullptr; }
    if (m_d3d12Device) { m_d3d12Device->Release(); m_d3d12Device = nullptr; }
    if (m_fence) { m_fence->Release(); m_fence = nullptr; }
    if (m_fenceEvent) { CloseHandle(m_fenceEvent); m_fenceEvent = nullptr; }

    m_imguiInitialized = false;
    LOG_DEBUG("ImGui DX12 backend shut down.");
}

void RenderHookDX12::CreateRenderTarget()
{
    m_mainRenderTargetDescriptor.resize(m_numBackBuffers);
    m_mainRenderTargetResource.resize(m_numBackBuffers);

    UINT rtvDescriptorSize = m_d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < m_numBackBuffers; i++)
    {
        m_mainRenderTargetDescriptor[i] = rtvHandle;
        m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_mainRenderTargetResource[i]));
        m_d3d12Device->CreateRenderTargetView(m_mainRenderTargetResource[i], nullptr, rtvHandle);
        rtvHandle.ptr += rtvDescriptorSize;
    }
}

void RenderHookDX12::CleanupRenderTarget()
{
    for (auto& resource : m_mainRenderTargetResource) {
        if (resource) resource->Release();
    }
    m_mainRenderTargetResource.clear();
    m_mainRenderTargetDescriptor.clear();
}

LRESULT CALLBACK RenderHookDX12::Hooked_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    auto pThis = s_instance;
    if (pThis && pThis->m_imguiInitialized)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        {
            ImGuiIO& io = ImGui::GetIO();
            if (io.WantCaptureMouse || io.WantCaptureKeyboard) {
                return TRUE;
            }
        }
    }
    
    if (pThis->m_handleInputCallback)
        pThis->m_handleInputCallback();

    if (pThis && pThis->m_original_WndProc)
    {
        return CallWindowProc(pThis->m_original_WndProc, hWnd, uMsg, wParam, lParam);
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

HRESULT WINAPI RenderHookDX12::Hooked_D3D12CreateDevice(IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, REFIID riid, void** ppDevice)
{
    auto pThis = s_instance;
    LOG_DEBUG("D3D12CreateDevice called, enabling debug layer...");

    ID3D12Debug* pDebugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController))))
    {
        pDebugController->EnableDebugLayer();
        pDebugController->Release();
        LOG_DEBUG("D3D12 Debug Layer ENABLED.");
    }
    else
    {
        LOG_DEBUG("Failed to get D3D12 Debug Interface. Is Graphics Tools installed?");
    }

    return pThis->m_original_D3D12CreateDevice(pAdapter, MinimumFeatureLevel, riid, ppDevice);
}

HRESULT WINAPI RenderHookDX12::Hooked_Present(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags)
{
    auto pThis = s_instance;
    if (!pThis) return pThis->m_original_Present(pSwapChain, SyncInterval, Flags);

    // Initialization logic: only proceed when all components are captured.
    if (!pThis->m_imguiInitialized)
    {
        // Capture swapchain and device from this function
        if (pThis->m_swapChain != pSwapChain) {
            pThis->m_swapChain = pSwapChain;
            pSwapChain->GetDevice(IID_PPV_ARGS(&pThis->m_d3d12Device));
            LOG_DEBUG("Captured SwapChain and D3D12 Device.");
        }
        
        // If we have everything we need, initialize ImGui
        if (pThis->m_swapChain && pThis->m_d3d12Device && pThis->m_commandQueue)
        {
             pThis->InitializeImGui(pSwapChain);
        }
    }
    
    if (pThis->m_imguiInitialized)
    {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGuiIO& io = ImGui::GetIO();
        // Add an assertion to catch the problem immediately
        assert(io.DisplaySize.x > 0.0f && io.DisplaySize.y > 0.0f);
        ImGui::NewFrame();
        pThis->OnPresent(); // Call user's GUI drawing callback
        ImGui::Render();

        UINT backBufferIdx = pSwapChain->GetCurrentBackBufferIndex();
        ID3D12CommandAllocator* commandAllocator = pThis->m_commandAllocators[backBufferIdx];
        commandAllocator->Reset();

        ID3D12GraphicsCommandList* commandList = nullptr;
        pThis->m_d3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, NULL, IID_PPV_ARGS(&commandList));

        // Transition back buffer to render target state
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = pThis->m_mainRenderTargetResource[backBufferIdx];
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        commandList->ResourceBarrier(1, &barrier);

        // Set render target
        commandList->OMSetRenderTargets(1, &pThis->m_mainRenderTargetDescriptor[backBufferIdx], FALSE, nullptr);
        
        // Set descriptor heaps
        commandList->SetDescriptorHeaps(1, &pThis->m_srvDescriptorHeap);

        // Render ImGui draw data
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

        // Transition back buffer to present state
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        commandList->ResourceBarrier(1, &barrier);

        commandList->Close();
        pThis->m_commandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&commandList);
        commandList->Release();
    }
    
    return pThis->m_original_Present(pSwapChain, SyncInterval, Flags);
}

HRESULT WINAPI RenderHookDX12::Hooked_ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    auto pThis = s_instance;
    if (pThis && pThis->m_imguiInitialized)
    {
        LOG_DEBUG("ResizeBuffers called. Cleaning up ImGui render targets.");
        pThis->CleanupRenderTarget();
    }

    HRESULT hr = pThis->m_original_ResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    
    if (SUCCEEDED(hr) && pThis && pThis->m_imguiInitialized)
    {
        LOG_DEBUG("Recreating ImGui render targets for new swap chain buffers.");
        pThis->m_numBackBuffers = BufferCount;
        pThis->CreateRenderTarget();
    }

    return hr;
}
void WINAPI RenderHookDX12::Hooked_ExecuteCommandLists(ID3D12CommandQueue* pQueue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists)
{
    auto pThis = s_instance;
    if (!pThis) return;

    // Capture the command queue on the first call
    if (!pThis->m_commandQueue)
    {
        LOG_DEBUG("Captured game's D3D12 Command Queue: %p", pQueue);
        pThis->m_commandQueue = pQueue;
    }

    // Always call the original function
    pThis->m_original_ExecuteCommandLists(pQueue, NumCommandLists, ppCommandLists);
}

void RenderHookDX12::SetGUICallback(GuiRenderCallback callback) {
    m_guiCallback = std::move(callback);
}

void RenderHookDX12::SetHandleInputCallback(HandleInputCallback callback) {
    m_handleInputCallback = std::move(callback);
}