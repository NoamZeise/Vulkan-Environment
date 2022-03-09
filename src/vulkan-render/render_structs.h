#ifndef VULKAN_RENDER_STRUCTS_H
#define VULKAN_RENDER_STRUCTS_H

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


struct Base
{
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	QueueFamilies queue;
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
};

struct AttachmentImage
{
	VkImage image;
	VkImageView view;
	VkDeviceMemory memory;
	VkFormat format;
};


struct SwapChain
{
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	VkSurfaceFormatKHR format;
	VkExtent2D swapchainExtent;
	VkExtent2D offscreenExtent;

	AttachmentImage depthBuffer;
	AttachmentImage multisampling;
	AttachmentImage offscreen;
	VkSampleCountFlagBits maxMsaaSamples;

	VkFramebuffer offscreenFramebuffer;
	std::vector<FrameData> frameData;
	std::vector<VkSemaphore> imageAquireSem;
};



#endif
