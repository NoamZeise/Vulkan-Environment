#ifndef DESCRIPTOR_SETS_H
#define DESCRIPTOR_SETS_H

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


namespace DS
{

struct viewProjection
{
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct lighting
{
	lighting()
	{
		ambient = glm::vec4(1.0f, 1.0f, 1.0f, 0.4f);
		directionalCol = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	    directionalVec = glm::vec4(-0.8f, -1.0f, -2.0f, 0.0f);
	}

	alignas(16) glm::vec4 ambient;
	alignas(16) glm::vec4 directionalCol;
	alignas(16) glm::vec4 directionalVec;
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
	std::vector<VkDescriptorPoolSize> poolSize;
	uint32_t binding;
};

} //end DS namespace

struct UniformBufferMemory
{
	VkBuffer buffer;
	VkDeviceSize memSize;
	VkDeviceSize slotSize;
	VkDeviceMemory memory;
	void* pointer;
};

struct memoryObjects
{
	UniformBufferMemory viewProj;
	UniformBufferMemory lighting;
};


#endif