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
#endif //NDEBUG

struct initVulkan
{
public:
	static void instance(VkInstance* instance);
	static void device(VkInstance instance, VkPhysicalDevice& device, VkDevice* logicalDevice, VkSurfaceKHR surface, QueueFamilies* families);
	static void swapChain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, SwapChain* swapchain, GLFWwindow* window, uint32_t graphicsQueueIndex);
	static void destroySwapchain(SwapChain* swapchain, const VkDevice& device);
	static void renderPass(VkDevice device, VkRenderPass* renderPass, SwapChain swapchain);
	static void framebuffers(VkDevice device, SwapChain* swapchain, VkRenderPass renderPass);
	static void graphicsPipeline(VkDevice device, Pipeline* pipeline, SwapChain swapchain, VkRenderPass renderPass,
	std::vector<DS::DescriptorSet*> descriptorSets, 
	std::vector<VkPushConstantRange> pushConstantsRanges,
	std::string vertexShaderPath, std::string fragmentShaderPath);
	static void CreateDescriptorSetLayout(VkDevice device, DS::DescriptorSet* descriptorSets,
		 std::vector<VkDescriptorType> descriptorTypes, std::vector<uint32_t> descriptorCount, 
		 VkShaderStageFlagBits stageFlags);
#ifndef NDEBUG
	static void debugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT* messenger);
	static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);
#endif

private:

	static void fillFrameData(VkDevice device, FrameData* frame, uint32_t graphicsQueueIndex);
	static void destroySwapchain(SwapChain* swapchain, const VkDevice& device, const VkSwapchainKHR& oldSwapChain);
	static VkShaderModule loadShaderModule(VkDevice device, std::string file);
	static void createDepthBuffer(VkDevice device, VkPhysicalDevice physicalDevice, SwapChain* swapchain);
	static void createMultisamplingBuffer(VkDevice device, VkPhysicalDevice physicalDevice, SwapChain* swapchain);
	static void createAttachmentImageResources(VkDevice device, VkPhysicalDevice physicalDevice, AttachmentImage* attachIm, SwapChain& swapchain, VkImageUsageFlags usage, VkImageAspectFlags imgAspect);
	static VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features);
	static void destroyAttachmentImageResources(VkDevice device, AttachmentImage attachment);
	//DEBUG MEMBERS
#ifndef NDEBUG

	static bool validationLayersSupported();

	static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* createInfo);

	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

#endif // NDEBUG
};



#endif
