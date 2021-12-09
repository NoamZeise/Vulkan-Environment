#pragma once
#ifndef USER_STRUCTS_H
#define USER_STRUCTS_H

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>

#include <stdint.h>
#include <vector>
#include <array>
#include <string>

//#define NDEBUG //uncomment for release mode

const bool USE_SRGB = false;
const bool ENABLE_MIP = true;
const bool PIXELATED = false; //for pixelated
const bool FIXED_RATIO = false;
const bool VSYNC = true;
const bool USE_MULTISAMPLING = true;
const bool USE_SAMPLE_SHADING = true;
#ifndef NDEBUG
const bool ERROR_ONLY = true;
#endif

const int TARGET_WIDTH = 1920;
const int TARGET_HEIGHT = 1080;

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
	VkExtent2D extent;

	AttachmentImage depthBuffer;
	AttachmentImage multisampling;
	VkSampleCountFlagBits maxMsaaSamples;


	std::vector<FrameData> frameData;
	std::vector<VkSemaphore> imageAquireSem;
};



#endif
