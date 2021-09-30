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


static bool USE_SRGB = false;
static bool ENABLE_MIP = false;
static bool PIXELATED = true; //for pixelated 

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
	glm::vec2 position;
	glm::vec2 texCoord;

	static std::array<VkVertexInputBindingDescription, 1> bindingDescriptions()
	{
		std::array<VkVertexInputBindingDescription, 1> bindingDescriptions;
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescriptions;
	}

	static std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		//position
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; //glm::vec3
		attributeDescriptions[0].offset = offsetof(Vertex, position);
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT; //glm::vec3
		attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

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
	uint32_t texID;
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