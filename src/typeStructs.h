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

#define NDEBUG //uncomment for release mode

const bool USE_SRGB = false;
const bool ENABLE_MIP = false;
const bool PIXELATED = true; //for pixelated
const bool FIXED_RATIO = true;
const bool VSYNC = true;
#ifndef NDEBUG
const bool ERROR_ONLY = false;
#endif

const int TARGET_WIDTH = 256;
const int TARGET_HEIGHT = 224;

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


struct SwapChain
{
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	VkSurfaceFormatKHR format;
	VkExtent2D extent;
	std::vector<FrameData> frameData;
	std::vector<VkSemaphore> imageAquireSem;
};

struct Pipeline
{
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout layout;
	VkPipeline pipeline;
};


struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec3 TexCoord;

	static std::array<VkVertexInputBindingDescription, 1> bindingDescriptions()
	{
		std::array<VkVertexInputBindingDescription, 1> bindingDescriptions;
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescriptions;
	}

	static std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

		//position
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, Position);
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; 
		attributeDescriptions[2].offset = offsetof(Vertex, Normal);
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT; 
		attributeDescriptions[2].offset = offsetof(Vertex, TexCoord);

		return attributeDescriptions;
	}
};

struct vectPushConstants
{
	glm::mat4 model;
};

struct fragPushConstants
{
	glm::vec4 colour;
	glm::vec4 texOffset;
	unsigned int TexID;
};


struct viewProjectionBufferObj
{
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct UniformBufferTypes
{
	VkBuffer buffer;
	VkDeviceSize memSize;
	VkDeviceSize slotSize;
	VkDeviceMemory memory;
	void* pointer;
};

struct memoryObjects
{
	VkDeviceMemory memory;
	VkBuffer vertexBuffer;
	VkBuffer indexBuffer;

	VkDeviceMemory stagingMemory;
	VkBuffer stagingBuffer;

	UniformBufferTypes viewProj;

};


struct DescriptorSets
{
	void destroySet(VkDevice device)
	{
		vkDestroyDescriptorPool(device, pool, nullptr);
		vkDestroyDescriptorSetLayout(device, layout, nullptr);
	}
	VkDescriptorPool pool;
	VkDescriptorSetLayout layout;
	std::vector<VkDescriptorSet> sets;
};


#endif
