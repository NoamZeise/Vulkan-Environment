#ifndef VULKAN_RENDER_STRUCTS_H
#define VULKAN_RENDER_STRUCTS_H

#include "vulkan/vulkan_core.h"
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include <stdint.h>
#include <vector>

struct QueueFamilies
{
	uint32_t graphicsPresentFamilyIndex;
	VkQueue graphicsPresentQueue;
};

struct EnabledFeatures
{
  bool samplerAnisotropy = false;
  bool sampleRateShading = false;
};

struct Base
{
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	QueueFamilies queue;
    EnabledFeatures features;
};


struct AttachmentImage
{
	VkImage image;
	VkImageView view;
	VkFormat format;
	size_t memoryOffset;
};


struct FrameData
{
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
	VkSemaphore presentReadySem;
	VkFence frameFinishedFen;

	VkImage image;
	VkImageView view;
	VkFramebuffer framebuffer;

	AttachmentImage multisampling;
	AttachmentImage depthBuffer;
	AttachmentImage offscreen;
	VkFramebuffer offscreenFramebuffer;
};


struct SwapChain
{
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	VkSurfaceFormatKHR format;
	VkExtent2D swapchainExtent;
	VkExtent2D offscreenExtent;
	VkSampleCountFlagBits maxMsaaSamples;
	std::vector<FrameData> frameData;
	std::vector<VkSemaphore> imageAquireSem;
	VkDeviceMemory attachmentMemory;
};



#endif
