#ifndef VK_H
#define VK_H

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include <array>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <set>
#include <string>
#include <fstream>
#include <cstring>

#include "config.h"
#include "render_structs.h"
#include "pipeline.h"
#include "descriptor_sets.h"
#include "vkhelper.h"

#ifndef NDEBUG

const std::array<const char*, 1> OPTIONAL_LAYERS = {
		"VK_LAYER_KHRONOS_validation"
};
const std::array<VkValidationFeatureEnableEXT, 2> VALIDATION_LAYER_FEATURES = {
	VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
	VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
};

const std::array<const char*, 1> REQUESTED_DEVICE_EXTENSIONS = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
#else //NDEBUG
const std::array<const char*, 1> REQUESTED_DEVICE_EXTENSIONS = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
#endif 

struct initVulkan
{
public:
	static void Instance(VkInstance* instance);
	static void Device(VkInstance instance, VkPhysicalDevice& device, VkDevice* logicalDevice, VkSurfaceKHR surface, QueueFamilies* families);
	static void Swapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, SwapChain* swapchain, GLFWwindow* window, uint32_t graphicsQueueIndex);
	static void DestroySwapchain(SwapChain* swapchain, const VkDevice& device);
	static void RenderPass(VkDevice device, VkRenderPass* renderPass, SwapChain swapchain, bool presentOnly);
	static void Framebuffers(VkDevice device, SwapChain* swapchain, VkRenderPass renderPass, bool presentOnly);
	static void GraphicsPipeline(VkDevice device, Pipeline* pipeline, SwapChain swapchain, VkRenderPass renderPass,
									std::vector<DS::DescriptorSet*> descriptorSets,
									std::vector<VkPushConstantRange> pushConstantsRanges,
									std::string vertexShaderPath, std::string fragmentShaderPath, bool useDepthTest, bool presentOnly);

	static void DescriptorSetAndLayout(VkDevice device, DS::DescriptorSet &ds, std::vector<DS::Binding*> bindings, VkShaderStageFlagBits stageFlags, size_t setCount);
	static void PrepareShaderBufferSets(Base base,	std::vector<DS::Binding*> ds, VkBuffer* buffer, VkDeviceMemory* memory);
#ifndef NDEBUG
	static void DebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT* messenger);
	static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);
#endif

private:

	static void _fillFrameData(VkDevice device, FrameData* frame, uint32_t graphicsQueueIndex);
	static void _destroySwapchain(SwapChain* swapchain, const VkDevice& device, const VkSwapchainKHR& oldSwapChain);
	static VkShaderModule _loadShaderModule(VkDevice device, std::string file);
	static void _createDepthBuffer(VkDevice device, VkPhysicalDevice physicalDevice, SwapChain* swapchain);
	static void _createMultisamplingBuffer(VkDevice device, VkPhysicalDevice physicalDevice, SwapChain* swapchain);
	static void _createAttachmentImageResources(VkDevice device, VkPhysicalDevice physicalDevice, AttachmentImage* attachIm, 
				SwapChain& swapchain, VkImageUsageFlags usage, VkImageAspectFlags imgAspect, VkSampleCountFlagBits samples);
	static VkFormat _findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features);
	static void _destroyAttachmentImageResources(VkDevice device, AttachmentImage attachment);
	static size_t _createHostVisibleShaderBufferMemory(Base base,	std::vector<DS::Binding*> ds, VkBuffer* buffer, VkDeviceMemory* memory);
	static void _createDescriptorSetLayout(VkDevice device, DS::DescriptorSet &ds, std::vector<DS::Binding*> &bindings, VkShaderStageFlagBits stageFlags);
	static void _createDescriptorSet(VkDevice device, DS::DescriptorSet &ds, size_t setCount);
	//DEBUG MEMBERS
#ifndef NDEBUG

	static bool _validationLayersSupported();

	static void _populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* createInfo);

	static VkResult _createDebugUtilsMessengerEXT(VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);

	static VKAPI_ATTR VkBool32 VKAPI_CALL _debugUtilsMessengerCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

#endif // NDEBUG
};



#endif
