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
		ambient = glm::vec4(1.0f, 1.0f, 1.0f, 0.2f);
		directionalCol = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	    directionalVec = glm::vec4(0.8f, -0.7f, -0.9f, 0.0f);
	}

	alignas(16) glm::vec4 ambient;
	alignas(16) glm::vec4 directionalCol;
	alignas(16) glm::vec4 directionalVec;
};

struct DescriptorSet
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
};

struct UniformBufferSet
{
	DescriptorSet  ds;
	
	size_t setCount;
	size_t dsStructSize;

	size_t offset;
	VkDeviceSize slotSize;
	void* pointer;

	void setPerUboProperties(size_t setCount, size_t dsStructSize)
	{
		this->setCount = setCount;
		this->dsStructSize = dsStructSize;
	}
	void storeSetData(size_t frameIndex, void* data)
	{
		std::memcpy(static_cast<char*>(pointer) + offset + (frameIndex * slotSize), data, dsStructSize);
	}
};

} //end DS namespace


#endif