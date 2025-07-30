#pragma once
#include "i_renderhook.h"

#include <Windows.h>

#include <string>
#include <vector>
#include <vulkan/vulkan.h>


class RenderHookVulkan : public IRenderHook
{
public:
	// Interface methods:
	RenderHookVulkan();
	~RenderHookVulkan();
	void Shutdown() override;
	void OnPresent() override;
	bool HookGraphicsAPI() override;
	void UnhookGraphicsAPI() override;
	void SetGUICallback(GuiRenderCallback callback) override;
	void SetHandleInputCallback(HandleInputCallback callback) override;

private:	
	bool HookVulkanFunction(HMODULE vulkanLib, const std::string& targetFunctionName, LPVOID detourFunction, LPVOID* ppOriginal);
	// Vulkan initialization and cleanup:
	VkResult CreateRenderPass();
	VkResult CreateDescriptorPool();

	// ImGui initialization and cleanup:
	void InitializeImGui();
	void DeinitializeImGui();

	// Hooked Vulkan functions:
	static VkResult VKAPI_PTR Hooked_vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo);
	static VkResult VKAPI_PTR Hooked_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice);
	static VkResult VKAPI_PTR Hooked_vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain);
	static void VKAPI_PTR Hooked_vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR pSwapchain, const VkAllocationCallbacks* pAllocator);
	static VkResult VKAPI_PTR Hooked_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance);
	static PFN_vkVoidFunction VKAPI_PTR Hooked_vkGetDeviceProcAddr(VkDevice device, const char* pName);

	// Hooked WndProc function:
	static LRESULT CALLBACK Hooked_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lPARAM);

private:
	// Vulkan function pointers:
	PFN_vkQueuePresentKHR m_original_vkQueuePresentKHR = nullptr;
	PFN_vkCreateDevice m_original_vkCreateDevice = nullptr;
	PFN_vkGetDeviceProcAddr m_original_vkGetDeviceProcAddr = nullptr;
	PFN_vkCreateSwapchainKHR m_original_vkCreateSwapchainKHR = nullptr;
	PFN_vkDestroySwapchainKHR m_original_vkDestroySwapchainKHR = nullptr;
	PFN_vkCreateInstance m_original_vkCreateInstance = nullptr;

	// Global Vulkan objects captured from the game
	VkDevice m_vkDevice = VK_NULL_HANDLE;
	VkQueue m_vkQueue = VK_NULL_HANDLE;
	VkInstance m_vkInstance = VK_NULL_HANDLE;
	VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;

	// ImGui related Vulkan objects
	VkDescriptorPool m_imguiDescriptorPool = VK_NULL_HANDLE;
	VkRenderPass m_imguiRenderPass = VK_NULL_HANDLE;
	std::vector<VkCommandPool> m_imguiCommandPools;
	std::vector<VkFramebuffer> m_imguiFramebuffers;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;
	VkFormat m_swapchainImageFormat = VK_FORMAT_UNDEFINED;
	VkExtent2D m_swapchainExtent = VKAPI_ATTR VkExtent2D{ 0, 0 }; 
	VkSwapchainKHR m_vkSwapchain = VK_NULL_HANDLE;

	// Synchronization objects for ImGui rendering
	std::vector<VkSemaphore> m_renderCompleteSemaphores;
	std::vector<VkFence> m_renderFences;

	bool m_imguiInitialized = false;
	bool m_vulkanDeviceCreated = false;


	GuiRenderCallback m_guiCallback;
	HandleInputCallback m_handleInputCallback;

	// Win32 Objects:
	HWND m_hWnd = nullptr;
	WNDPROC m_original_WndProc = nullptr;

	static RenderHookVulkan* s_instance;
};