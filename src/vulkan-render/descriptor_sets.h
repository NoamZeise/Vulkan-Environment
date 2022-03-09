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
#include <stdexcept>

#ifdef __linux__
	#include <cstring>
#endif

namespace DS
{
namespace ShaderStructs
{
struct viewProjection
{
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct Per3DInstance
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 normalMat;
};

struct Per2DVert
{
	alignas(16) glm::mat4 model;
};

struct Per2DFrag
{
	alignas(16) glm::vec4 colour;
	alignas(16) glm::vec4 texOffset;
	alignas(4) uint32_t texID;
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

} //end shader struct namespace

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


struct Binding
{
	DescriptorSet* ds;
	VkDescriptorType type;

	size_t setCount;
	size_t dataStructSize;
	size_t binding = 0;
	size_t descriptorCount = 1;

	size_t offset;
	VkDeviceSize slotSize;
	void* pointer;
	VkImageView* imageViews;
	VkSampler* samplers;

	void storeSetData(size_t frameIndex, void* data, size_t index)
	{
		if(type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
#ifdef __linux__
			memcpy(static_cast<char*>(pointer) + offset + ((frameIndex * slotSize * descriptorCount) + (index * slotSize)) , data, dataStructSize);
#else
			std::memcpy(static_cast<char*>(pointer) + offset + ((frameIndex * slotSize * descriptorCount) + (index * slotSize)) , data, dataStructSize);
#endif
		else
			throw std::runtime_error("Descriptor Shader Buffer: tried to store data in non uniform or storage buffer!");
	}
};

template <typename T>
struct BindingAndData
{
	Binding binding;
	std::vector<T> data;

	void setBufferProps(size_t setCount, VkDescriptorType type, DescriptorSet* set, uint32_t dataCount, VkImageView* pImgViews, VkSampler* pSamplers)
	{
		data.resize(dataCount);
		binding.ds = set;
		binding.descriptorCount = dataCount;
		binding.setCount = setCount;
		binding.dataStructSize = sizeof(T);
		binding.type = type;
		binding.imageViews = pImgViews;
		binding.samplers = pSamplers;

		if(binding.type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE && pImgViews == nullptr)
			throw std::runtime_error("Descriptor Set Binding: type is sampled image but pImgViews was a nullptr!");
		if(binding.type == VK_DESCRIPTOR_TYPE_SAMPLER && pSamplers == nullptr)
			throw std::runtime_error("Descriptor Set Binding: type is sampler image but pSamplers was a nullptr!");
	}

	void setBufferProps(size_t setCount, VkDescriptorType type, DescriptorSet* set, uint32_t dataCount)
	{
		setBufferProps(setCount, type, set, dataCount, nullptr, nullptr);
	}
	void storeData(size_t frameIndex)
	{
		for(size_t i = 0 ; i < data.size(); i++)
			storeData(frameIndex, i);
	}

	void storeData(size_t frameIndex, size_t arrayIndex)
	{
		if(arrayIndex >= data.size())
			throw std::runtime_error("Descriptor Set Binding: tried to store data in an index outside of the data range");
		binding.storeSetData(frameIndex, &data[arrayIndex], arrayIndex);
	}

};

} //end DS namespace


#endif
