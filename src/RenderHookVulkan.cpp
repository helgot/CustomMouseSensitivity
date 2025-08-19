#include "RenderHookVulkan.h"

#include "IRenderHook.h"
#include "logger.h"
#include <MinHook.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);                

RenderHookVulkan* RenderHookVulkan::s_instance = nullptr;

constexpr bool kVkEnableValidationLayers = false;

RenderHookVulkan::RenderHookVulkan()
{
	s_instance = this;
}

RenderHookVulkan::~RenderHookVulkan() {
	Shutdown();
	s_instance = nullptr;
}

void RenderHookVulkan::Shutdown()
{
	DeinitializeImGui();
	UnhookGraphicsAPI();
}

void RenderHookVulkan::OnPresent()
{
	if (m_guiCallback)
    {
		m_guiCallback();
	} else
    {
        LOG_DEBUG("No GUI callback set, skipping ImGui rendering.");
	}
}

PFN_vkVoidFunction VKAPI_PTR RenderHookVulkan::Hooked_vkGetDeviceProcAddr(VkDevice device, const char* pName)
{
    auto* hook = s_instance;
    LOG_DEBUG("vkGetDeviceProcAddr requested: %s", pName);

    // Intercept requests for specific functions and return our hooks instead.
    if (strcmp(pName, "vkCreateSwapchainKHR") == 0) {
        if (!hook->m_original_vkCreateSwapchainKHR) {
            hook->m_original_vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)hook->m_original_vkGetDeviceProcAddr(device, pName);
        }
        return (PFN_vkVoidFunction)Hooked_vkCreateSwapchainKHR;
    }
    if (strcmp(pName, "vkDestroySwapchainKHR") == 0) {
        if (!hook->m_original_vkDestroySwapchainKHR) {
            hook->m_original_vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)hook->m_original_vkGetDeviceProcAddr(device, pName);
        }
        return (PFN_vkVoidFunction)Hooked_vkDestroySwapchainKHR;
    }
    if (strcmp(pName, "vkQueuePresentKHR") == 0) {
        if (!hook->m_original_vkQueuePresentKHR) {
            hook->m_original_vkQueuePresentKHR = (PFN_vkQueuePresentKHR)hook->m_original_vkGetDeviceProcAddr(device, pName);
        }
        return (PFN_vkVoidFunction)Hooked_vkQueuePresentKHR;
    }

    // For all other functions, pass the request to the original.
    return hook->m_original_vkGetDeviceProcAddr(device, pName);
}

bool RenderHookVulkan::HookVulkanFunction(HMODULE vulkanLib, const std::string& targetFunctionName, LPVOID detourFunction, LPVOID* ppOriginal)
{
    // Hook vkCreateInstance (instance-level function, usually loaded directly)
    PFN_vkCreateInstance targetFunction = (PFN_vkCreateInstance)GetProcAddress(vulkanLib, targetFunctionName.c_str());
    if (targetFunction == NULL) {
        LOG_DEBUG("Failed to get address of %s.", targetFunctionName.c_str());
        MH_Uninitialize();
        return false;
    }
    
    if (MH_CreateHook(targetFunction, detourFunction, ppOriginal) != MH_OK) {
        LOG_DEBUG("Failed to create hook for %s!", targetFunctionName.c_str());
		LOG_DEBUG("Address of target function: %p", static_cast<void*>(targetFunction));
        MH_Uninitialize();
        return false;
    }
    if (MH_EnableHook(targetFunction) != MH_OK) {
        LOG_DEBUG("Failed to enable hook for %s!", targetFunctionName.c_str());
        MH_Uninitialize();
        return false;
    }
    LOG_DEBUG("Hook for %s enabled.", targetFunctionName.c_str());

    LOG_DEBUG("Address of target function: %p", (void*)targetFunction);
	LOG_DEBUG("Address of detour function: %p", (void*)detourFunction);
	LOG_DEBUG("Address of original function: %p", (void*)(*ppOriginal));

    return true;
}

bool RenderHookVulkan::HookGraphicsAPI()
{
    // Get the address of vulkan-1.dll
    HMODULE vulkanLib = GetModuleHandle("vulkan-1.dll");
    if (vulkanLib == NULL) {
        LOG_DEBUG("Failed to get handle for vulkan-1.dll!");
        MH_Uninitialize();
        return false;
    }

    if (!HookVulkanFunction(vulkanLib, "vkCreateInstance", RenderHookVulkan::Hooked_vkCreateInstance, (LPVOID*)&m_original_vkCreateInstance))
        return false;
    if (!HookVulkanFunction(vulkanLib, "vkCreateDevice", RenderHookVulkan::Hooked_vkCreateDevice, (LPVOID*)&m_original_vkCreateDevice))
        return false;
    if (!HookVulkanFunction(vulkanLib, "vkGetDeviceProcAddr", RenderHookVulkan::Hooked_vkGetDeviceProcAddr, (LPVOID*)&m_original_vkGetDeviceProcAddr))
        return false;

    LOG_DEBUG("Vulkan API hooks installed successfully.");
    return true;
}

void RenderHookVulkan::UnhookGraphicsAPI()
{
    MH_DisableHook(m_original_vkCreateInstance);
    MH_DisableHook(m_original_vkCreateDevice);
    MH_DisableHook(m_original_vkGetDeviceProcAddr);

    LOG_DEBUG("Vulkan Hooks Disabled.");
}

VkResult RenderHookVulkan::CreateRenderPass()
{
    LOG_DEBUG("%s: Called.", __func__);
    LOG_DEBUG("%s: g_vkDevice at entry: %p", __func__, (void*)m_vkDevice); // LOG_DEBUG g_vkDevice value

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    // We MUST load the game's content to render over it.
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; 
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; 

    // Tell the pass to transition the image to the layout needed for presentation when we are done.
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (m_vkDevice == VK_NULL_HANDLE) {
        LOG_DEBUG("%s: Error: Vulkan device is not initialized. Cannot create ImGui render pass.", __func__);
        return VkResult::VK_ERROR_DEVICE_LOST;
    }
    LOG_DEBUG("%s: Attempting to create render pass with VkDevice: %p", __func__, (void*)m_vkDevice);
    return vkCreateRenderPass(m_vkDevice, &renderPassInfo, nullptr, &m_imguiRenderPass);
}

VkResult RenderHookVulkan::CreateDescriptorPool()
{
    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    return vkCreateDescriptorPool(m_vkDevice, &pool_info, nullptr, &m_imguiDescriptorPool);
}

void RenderHookVulkan::InitializeImGui()
{
    LOG_DEBUG("%s: Called.", __func__);
    if (m_imguiInitialized) {
        LOG_DEBUG("%s: ImGui already initialized.", __func__);
        return;
    }
    if (m_vkInstance == VK_NULL_HANDLE || m_vkDevice == VK_NULL_HANDLE || m_vkPhysicalDevice == VK_NULL_HANDLE || m_vkQueue == VK_NULL_HANDLE || m_swapchainImages.empty() || m_imguiRenderPass == VK_NULL_HANDLE) {
        LOG_DEBUG("%s: ImGui Vulkan backend initialization skipped: missing required Vulkan objects (Instance: %p, PhysicalDevice: %p, Device: %p, Queue: %p, SwapchainImages count: %zu, RenderPass: %p).",
            __func__, (void*)m_vkInstance, (void*)m_vkPhysicalDevice, (void*)m_vkDevice, (void*)m_vkQueue, m_swapchainImages.size(), (void*)m_imguiRenderPass);
        return;
    }

    LOG_DEBUG("Initializing ImGui Vulkan backend...");

    // 2. Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange; // Don't change mouse cursor

    // 3. Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // 4. Setup Platform backends
	{ //PLATFORM SPECIFIC CODE... Could replace with a function pointer or similar to avoid platform-specific code here.
        // 1. Get window handle
        m_hWnd = FindWindow(NULL, "Red Dead Redemption 2");
        if (m_hWnd == NULL) {
            LOG_DEBUG("Could not find game window handle. Make sure the game is running and the window title is correct.");
            return;
        }
        else {
            LOG_DEBUG("Found game window handle: %p", m_hWnd);
            // Replace the WndProc and store the original one
            m_original_WndProc = (WNDPROC)SetWindowLongPtrA(m_hWnd, GWLP_WNDPROC, (LONG_PTR)RenderHookVulkan::Hooked_WndProc);
            if (m_original_WndProc) {
                LOG_DEBUG("Successfully hooked WndProc.");
            }
            else {
                LOG_DEBUG("Failed to hook WndProc!");
            }
        }
        ImGui_ImplWin32_Init(m_hWnd);
    }

    // Create ImGui descriptor pool if not already created (e.g., in vkCreateSwapchainKHR hook)
    if (m_imguiDescriptorPool == VK_NULL_HANDLE) {
        if (CreateDescriptorPool() != VK_SUCCESS) {
            LOG_DEBUG("Error: Failed to create ImGui descriptor pool!");
            return;
        }
    }

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_vkInstance; // g_vkInstance needs to be captured (e.g., from vkCreateInstance hook)
    init_info.PhysicalDevice = m_vkPhysicalDevice;
    init_info.Device = m_vkDevice;
    // Assuming queue family 0 is graphics and present. You might need to find the correct one dynamically.
    init_info.QueueFamily = 0;
    init_info.Queue = m_vkQueue;
    init_info.PipelineCache = VK_NULL_HANDLE; // Can be VK_NULL_HANDLE
    init_info.DescriptorPool = m_imguiDescriptorPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = (uint32_t)m_swapchainImages.size(); // Use actual swapchain image count
    init_info.ImageCount = (uint32_t)m_swapchainImages.size();     // Use actual swapchain image count
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = [](VkResult err) {
        if (err != VK_SUCCESS) {
            LOG_DEBUG("Vulkan ImGui Error: VkResult = %d", err);
            // Optionally, you can throw an exception or assert here
        }
        };
    // Set the RenderPass in the init_info structure as per the latest ImGui API
    init_info.RenderPass = m_imguiRenderPass;
    // Set API Version (0 will default to ImGui's internal default, typically VK_API_VERSION_1_0)
    init_info.ApiVersion = 0; // Or VK_API_VERSION_1_0, VK_API_VERSION_1_1, etc.
    // We are using a fixed RenderPass, so dynamic rendering is not enabled.
    init_info.UseDynamicRendering = false;

    ImGui_ImplVulkan_Init(&init_info); // Removed the direct RenderPass parameter

    // Font texture upload is now handled automatically by ImGui_ImplVulkan_NewFrame()
    // The previous manual font upload code has been removed as per ImGui changeLOG_DEBUG.

    m_imguiInitialized = true;
    LOG_DEBUG("ImGui Vulkan backend initialized successfully.");
}

void RenderHookVulkan::DeinitializeImGui()
{
    if (!m_imguiInitialized) {
        return;
    }

    LOG_DEBUG("Shutting down ImGui Vulkan backend...");

    vkDeviceWaitIdle(m_vkDevice); // Ensure all GPU operations are complete

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();

    ImGui::DestroyContext();

    if (m_imguiRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_vkDevice, m_imguiRenderPass, nullptr);
        m_imguiRenderPass = VK_NULL_HANDLE;
    }
    for (auto& fb : m_imguiFramebuffers) {
        if (fb != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(m_vkDevice, fb, nullptr);
        }
    }
    m_imguiFramebuffers.clear();
    for (auto& iv : m_swapchainImageViews) {
        if (iv != VK_NULL_HANDLE) {
            vkDestroyImageView(m_vkDevice, iv, nullptr);
        }
    }
    m_swapchainImageViews.clear();

    // Destroy command pools
    for (auto& pool : m_imguiCommandPools) {
        if (pool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(m_vkDevice, pool, nullptr);
        }
    }
    m_imguiCommandPools.clear();

    if (m_imguiDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_vkDevice, m_imguiDescriptorPool, nullptr);
        m_imguiDescriptorPool = VK_NULL_HANDLE;
    }

    for (auto& semaphore : m_renderCompleteSemaphores) {
        vkDestroySemaphore(m_vkDevice, semaphore, nullptr);
    }
    m_renderCompleteSemaphores.clear();

    for (auto& fence : m_renderFences) {
        vkDestroyFence(m_vkDevice, fence, nullptr);
    }
    m_renderFences.clear();


    m_imguiInitialized = false;
    LOG_DEBUG("ImGui Vulkan backend shut down.");
}

LRESULT CALLBACK RenderHookVulkan::Hooked_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    auto pThis = static_cast<RenderHookVulkan*>(s_instance);
    // For custom application messages, pass them directly to the game's original handler
    // to prevent ImGui from interfering with sensitive, proprietary message loops.
    if (uMsg >= WM_APP) {
        if (pThis && pThis->m_original_WndProc) {
            return CallWindowProc(pThis->m_original_WndProc, hWnd, uMsg, wParam, lParam);
        }
    }

    // For all standard messages, let ImGui process them first.
    if (pThis && pThis->m_imguiInitialized && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
        // If ImGui wants to capture input, we should absorb the message and not pass it to the game.
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse || io.WantCaptureKeyboard) {
            return TRUE;
        }

    }

    if(pThis->m_handleInputCallback)
        pThis->m_handleInputCallback();

    // If we're here, it means ImGui did not want to capture the input,
    // so we should pass the message to the original window procedure.
    if (pThis && pThis->m_original_WndProc) {
        return CallWindowProc(pThis->m_original_WndProc, hWnd, uMsg, wParam, lParam);
    }

    // Fallback if something went terribly wrong.
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

VkResult VKAPI_PTR RenderHookVulkan::Hooked_vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo)
{
    RenderHookVulkan* pThis = static_cast<RenderHookVulkan*>(s_instance);
    if (!pThis) return VK_ERROR_INITIALIZATION_FAILED;

    // --- (Initialization logic is fine) ---
    if (pThis->m_vkQueue == VK_NULL_HANDLE) {
        pThis->m_vkQueue = queue;
    }
    if (!pThis->m_imguiInitialized) {
        pThis->InitializeImGui();
    }

    // 判断菜单是否可见（UIManager::m_isMenuVisible），只有可见时才渲染 ImGui
    bool isMenuVisible = false;
    if (pThis->m_guiCallback) {
        // 通过 UIManager 的 Render 回调是否有内容判断
        // 这里假设 m_guiCallback 是 UIManager::Render，且 UIManager 内部有 m_isMenuVisible
        // 推荐将 UIManager::m_isMenuVisible 暴露为 public 或加一个 getter
        // 这里用静态变量 hack（实际建议用 getter）
        extern bool g_isMenuVisible;
        isMenuVisible = g_isMenuVisible;
    }

    if (pThis->m_imguiInitialized && pPresentInfo->waitSemaphoreCount > 0 && isMenuVisible)
    {
        VkPresentInfoKHR modifiedPresentInfo = *pPresentInfo;
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        pThis->OnPresent();
        ImGui::Render();

        ImDrawData* draw_data = ImGui::GetDrawData();
        uint32_t imageIndex = pPresentInfo->pImageIndices[0];

        VkFence currentFrameFence = pThis->m_renderFences[imageIndex];
        VkSemaphore currentFrameSemaphore = pThis->m_renderCompleteSemaphores[imageIndex];

        vkResetCommandPool(pThis->m_vkDevice, pThis->m_imguiCommandPools[imageIndex], 0);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = pThis->m_imguiCommandPools[imageIndex];
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer imguiCmdBuffer;
        if (vkAllocateCommandBuffers(pThis->m_vkDevice, &allocInfo, &imguiCmdBuffer) != VK_SUCCESS) {
            LOG_DEBUG("Error: Failed to allocate ImGui command buffer!");
            return pThis->m_original_vkQueuePresentKHR(queue, pPresentInfo);
        }

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(imguiCmdBuffer, &beginInfo);
        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = pThis->m_imguiRenderPass;
        renderPassBeginInfo.framebuffer = pThis->m_imguiFramebuffers[imageIndex];
        renderPassBeginInfo.renderArea.extent = pThis->m_swapchainExtent;

        vkCmdBeginRenderPass(imguiCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        ImGui_ImplVulkan_RenderDrawData(draw_data, imguiCmdBuffer);
        vkCmdEndRenderPass(imguiCmdBuffer);
        vkEndCommandBuffer(imguiCmdBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore gameRenderSemaphore = pPresentInfo->pWaitSemaphores[0];
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &gameRenderSemaphore;
        submitInfo.pWaitDstStageMask = &waitStage;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &imguiCmdBuffer;

        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &currentFrameSemaphore;

        vkResetFences(pThis->m_vkDevice, 1, &currentFrameFence);
        vkQueueSubmit(queue, 1, &submitInfo, currentFrameFence);

        vkWaitForFences(pThis->m_vkDevice, 1, &currentFrameFence, VK_TRUE, UINT64_MAX);

        vkFreeCommandBuffers(pThis->m_vkDevice, pThis->m_imguiCommandPools[imageIndex], 1, &imguiCmdBuffer);

        modifiedPresentInfo.pWaitSemaphores = &currentFrameSemaphore;
        return pThis->m_original_vkQueuePresentKHR(queue, &modifiedPresentInfo);
    }

    // 菜单关闭时，直接调用原始 Present，不做任何 ImGui 渲染和同步
    return pThis->m_original_vkQueuePresentKHR(queue, pPresentInfo);
}

VkResult VKAPI_PTR RenderHookVulkan::Hooked_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
    LOG_DEBUG("%s: Called.", __func__);
    RenderHookVulkan* pThis = static_cast<RenderHookVulkan*>(s_instance);
    VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures{};
    timelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
    timelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;

    // Important: You must chain this structure to the pNext pointer of VkDeviceCreateInfo.
    // Since we don't know what might already be in the pNext chain, we must preserve it.
    timelineSemaphoreFeatures.pNext = (void*)pCreateInfo->pNext; // Preserve existing pNext chain

    // Create a modifiable copy of the original create info.
    VkDeviceCreateInfo modifiedCreateInfo = *pCreateInfo;

    // Point its pNext to our new feature structure.
    modifiedCreateInfo.pNext = &timelineSemaphoreFeatures;

    // Call the original function with the MODIFIED create info.
    VkResult result = pThis->m_original_vkCreateDevice(physicalDevice, &modifiedCreateInfo, pAllocator, pDevice);
    if (result == VK_SUCCESS) {
        pThis->m_vkPhysicalDevice = physicalDevice;
        pThis->m_vkDevice = *pDevice;
        pThis->m_vulkanDeviceCreated = true;
        LOG_DEBUG("%s: Captured VkDevice: %p, VkPhysicalDevice: %p", __func__, (void*)pThis->m_vkDevice, (void*)pThis->m_vkPhysicalDevice);

        // Capture VkInstance (can be obtained from vkGetInstanceProcAddr or passed to vkCreateDevice)
        // For simplicity, assuming it's available globally or from a previous hook
        // In a real scenario, you'd likely hook vkCreateInstance as well to get g_vkInstance.
        // If not, you'd need to establish how to get it.
        // For now, let's try to get a queue from the created device.
        uint32_t queueFamilyIndex = 0; // Assuming the first queue family supports graphics and present
        vkGetDeviceQueue(pThis->m_vkDevice, queueFamilyIndex, 0, &pThis->m_vkQueue);
        LOG_DEBUG("%s: Captured VkQueue: %p", __func__, (void*)pThis->m_vkQueue);
    }
    return result;
}
void VKAPI_PTR RenderHookVulkan::Hooked_vkDestroySwapchainKHR(VkDevice device,  VkSwapchainKHR pSwapchain, const VkAllocationCallbacks* pAllocator)
{
    LOG_DEBUG("%s: Called.", __func__);
    RenderHookVulkan* pThis = static_cast<RenderHookVulkan*>(s_instance);
    s_instance->DeinitializeImGui();
    pThis->m_original_vkDestroySwapchainKHR(device, pSwapchain, pAllocator);
}

VkResult VKAPI_PTR RenderHookVulkan::Hooked_vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain)
{
    LOG_DEBUG("%s: Called.", __func__);
    RenderHookVulkan* pThis = static_cast<RenderHookVulkan*>(s_instance);
    VkResult result = pThis->m_original_vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);

    if (result == VK_SUCCESS) {
        pThis->m_vkSwapchain = *pSwapchain; // Store the created swapchain object
        LOG_DEBUG("%s: Swapchain created successfully. Capturing details...", __func__);

        // Store swapchain properties
        pThis->m_swapchainImageFormat = pCreateInfo->imageFormat;
        pThis->m_swapchainExtent = pCreateInfo->imageExtent;
        LOG_DEBUG("%s: Swapchain Format: %d, Extent: %dx%d", __func__, pThis->m_swapchainImageFormat, pThis->m_swapchainExtent.width, pThis->m_swapchainExtent.height);

        // Get swapchain images
        uint32_t imageCount;
        vkGetSwapchainImagesKHR(device, *pSwapchain, &imageCount, nullptr);
        pThis->m_swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, *pSwapchain, &imageCount, pThis->m_swapchainImages.data());
        LOG_DEBUG("%s: Captured %u swapchain images.", __func__, imageCount);

        // Resize vectors for image views, framebuffers, and command pools
        pThis->m_swapchainImageViews.resize(imageCount);
        pThis->m_imguiFramebuffers.resize(imageCount);
        pThis->m_imguiCommandPools.resize(imageCount);

        // In Hooked_vkCreateSwapchainKHR, after resizing the other vectors...
        pThis->m_renderCompleteSemaphores.resize(imageCount);
        pThis->m_renderFences.resize(imageCount);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Create signaled for first use

        for (uint32_t i = 0; i < imageCount; ++i) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &pThis->m_renderCompleteSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &pThis->m_renderFences[i]) != VK_SUCCESS) {
                LOG_DEBUG("Failed to create synchronization objects for frame %u!", i);
            }
        }
        LOG_DEBUG("Created per-frame synchronization objects.");
        // Create a simple render pass for ImGui (if not already created)
        // This is crucial for ImGui_ImplVulkan_Init and framebuffer creation
        if (pThis->m_vkDevice == VK_NULL_HANDLE) { // Double-check g_vkDevice here
            LOG_DEBUG("%s: CRITICAL ERROR: g_vkDevice is NULL before calling CreateImGuiRenderPass! Cannot create render pass.", __func__);
            // This indicates a severe issue in the Vulkan initialization flow or our hook order.
            // We cannot proceed with ImGui initialization if the device is not available.
            return result; // Return the original result, but ImGui won't initialize.
        }
        if (pThis->CreateRenderPass() != VK_SUCCESS) {
            LOG_DEBUG("%s: Error: Failed to create ImGui render pass during swapchain creation!", __func__);
            // Handle error, maybe return a failure result or log more severely
        }
        else {
            LOG_DEBUG("%s: ImGui render pass created.", __func__);
        }

        // Create image views, framebuffers, and command pools for each swapchain image
        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.format = pThis->m_swapchainImageFormat;
        imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = pThis->m_imguiRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.width = pThis->m_swapchainExtent.width;
        framebufferInfo.height = pThis->m_swapchainExtent.height;
        framebufferInfo.layers = 1;

        VkCommandPoolCreateInfo cmdPoolInfo{};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        // Assuming queue family 0 is graphics and present. You might need to find the correct one.
        cmdPoolInfo.queueFamilyIndex = 0;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        for (uint32_t i = 0; i < imageCount; ++i) {
            imageViewInfo.image = pThis->m_swapchainImages[i];
            if (vkCreateImageView(device, &imageViewInfo, nullptr, &pThis->m_swapchainImageViews[i]) != VK_SUCCESS) {
                LOG_DEBUG("%s: Error: Failed to create image view for swapchain image %u!", __func__, i);
                // Handle error
            }

            framebufferInfo.pAttachments = &pThis->m_swapchainImageViews[i];
            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &pThis->m_imguiFramebuffers[i]) != VK_SUCCESS) {
                LOG_DEBUG("%s: Error: Failed to create framebuffer for swapchain image %u!", __func__, i);
                // Handle error
            }

            if (vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &pThis->m_imguiCommandPools[i]) != VK_SUCCESS) {
                LOG_DEBUG("%s: Error: Failed to create command pool for ImGui command buffer %u!", __func__, i);
                // Handle error
            }
        }
        LOG_DEBUG("%s: Created image views, framebuffers, and command pools for ImGui.", __func__);

        // Initialize ImGui Vulkan backend now that we have all swapchain details
        // This should be called only once after all necessary Vulkan objects are available
        if (!pThis->m_imguiInitialized) {
            pThis->InitializeImGui();
        }
    }
    return result;
}

VkResult VKAPI_PTR RenderHookVulkan::Hooked_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
    LOG_DEBUG("%s: Called.", __func__);
    RenderHookVulkan* pThis = static_cast<RenderHookVulkan*>(s_instance);

    // Copy the original CreateInfo so we can modify it (if we would like to add validation). 
    VkInstanceCreateInfo modifiedCreateInfo = *pCreateInfo;

    if (kVkEnableValidationLayers)
    {
        // Copy the original enabled layers, if any
        std::vector<const char*> enabledLayers;
        if (pCreateInfo->enabledLayerCount > 0) {
            enabledLayers.assign(pCreateInfo->ppEnabledLayerNames,
                                 pCreateInfo->ppEnabledLayerNames + pCreateInfo->enabledLayerCount);
        }

        // Check whether validation layer is already in the list.
        const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
        auto it = std::find(enabledLayers.begin(), enabledLayers.end(), validationLayerName);
        if (it == enabledLayers.end()) {
            enabledLayers.push_back(validationLayerName);
            LOG_DEBUG("%s: Injected validation layer.", __func__);
        }
        // Update the modifiedCreateInfo to include new layers
        modifiedCreateInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
        modifiedCreateInfo.ppEnabledLayerNames = enabledLayers.data();

    }
    // Call original function with modified CreateInfo.
    VkResult result = pThis->m_original_vkCreateInstance(&modifiedCreateInfo, pAllocator, pInstance);

    if (result == VK_SUCCESS) {
        pThis->m_vkInstance = *pInstance;
        LOG_DEBUG("%s: Captured VkInstance: %p", __func__, (void*)pThis->m_vkInstance);
    }

    return result;
}


void RenderHookVulkan::SetGUICallback(GuiRenderCallback callback) {
    m_guiCallback = std::move(callback);
}

void RenderHookVulkan::SetHandleInputCallback(HandleInputCallback callback) {
    m_handleInputCallback = std::move(callback);
}


