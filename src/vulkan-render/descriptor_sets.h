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

const unsigned int MAX_BATCH_SIZE = 10000;

struct PerInstance
{
	alignas(16) glm::mat4 model[MAX_BATCH_SIZE];
	alignas(16) glm::mat4 normalMat[MAX_BATCH_SIZE];
};

struct lighting
{
	lighting()
	{
		ambient = glm::vec4(1.0f, 1.0f, 1.0f, 0.6f);
		diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 0.7f);
		specular = glm::vec4(0.1f, 0.1f, 0.1f, 5.0f);
	    direction = glm::vec4(0.3f, -0.3f, -0.5f, 0.0f);
	}

	alignas(16) glm::vec4 ambient;
	alignas(16) glm::vec4 diffuse;
	alignas(16) glm::vec4 specular;
	alignas(16) glm::vec4 direction;
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

enum class BufferType
{
	Uniform,
	Storage
};

struct ShaderBufferSet
{
	DescriptorSet  ds;
	BufferType type;
	
	size_t setCount;
	size_t dsStructSize;

	size_t offset;
	VkDeviceSize slotSize;
	void* pointer;

	void setPerUboProperties(size_t setCount, size_t dsStructSize, DS::BufferType type)
	{
		this->setCount = setCount;
		this->dsStructSize = dsStructSize;
		this->type = type;
	}
	void storeSetData(size_t frameIndex, void* data)
	{
		std::memcpy(static_cast<char*>(pointer) + offset + (frameIndex * slotSize), data, dsStructSize);
	}
};

} //end DS namespace


#endif