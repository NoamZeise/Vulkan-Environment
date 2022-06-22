#ifndef VULKAN_RENDER_STRUCTS_H
#define VULKAN_RENDER_STRUCTS_H

#include "vulkan/vulkan_core.h"
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include <stdint.h>
#include <vector>
#include "parts/images.h"

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

	void destroy(VkDevice device)
	{
		vkDestroyImageView(device, view, nullptr);
		vkDestroyImage(device, image, nullptr);
	}
};


struct FrameData
{
	void SetPerFramData(VkDevice device, VkImage image, VkFormat format, uint32_t graphicsQueueIndex)
	{
		this->image = image;
		part::create::ImageView(device, &view, image, format, VK_IMAGE_ASPECT_COLOR_BIT);
	        	//create command pool
		VkCommandPoolCreateInfo commandPoolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		commandPoolInfo.queueFamilyIndex = graphicsQueueIndex;
		//commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		if (vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool) != VK_SUCCESS)
			throw std::runtime_error("failed to create command buffer");

		//create command buffer
		VkCommandBufferAllocateInfo commandBufferInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		commandBufferInfo.commandPool = commandPool;
		commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferInfo.commandBufferCount = 1;
		if (vkAllocateCommandBuffers(device, &commandBufferInfo, &commandBuffer))
			throw std::runtime_error("failed to allocate command buffer");

		//create semaphores
		VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &presentReadySem) != VK_SUCCESS)
			throw std::runtime_error("failed to create present ready semaphore");

		//create fence
		VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		if (vkCreateFence(device, &fenceInfo, nullptr, &frameFinishedFen) != VK_SUCCESS)
			throw std::runtime_error("failed to create frame finished fence");
	}
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

	void destroyResources(VkDevice device)
	{
			for (size_t i = 0; i < frameData.size(); i++)
	{
		frameData[i].offscreen.destroy(device);
		frameData[i].depthBuffer.destroy(device);
		if(maxMsaaSamples != VK_SAMPLE_COUNT_1_BIT)
			frameData[i].multisampling.destroy(device);

		vkDestroyImageView(device, frameData[i].view, nullptr);
		vkFreeCommandBuffers(device, frameData[i].commandPool, 1, &frameData[i].commandBuffer);
		vkDestroyCommandPool(device, frameData[i].commandPool, nullptr);
		vkDestroySemaphore(device, frameData[i].presentReadySem, nullptr);
		vkDestroyFence(device, frameData[i].frameFinishedFen, nullptr);
	}

	vkFreeMemory(device,attachmentMemory, nullptr);

	for (size_t i = 0; i < imageAquireSem.size(); i++)
	{
		vkDestroySemaphore(device, imageAquireSem[i], nullptr);
	}
	imageAquireSem.clear();
	frameData.clear();
	}
};



#endif
