#include "vkhelper.h"

namespace vkhelper
{

uint32_t findMemoryIndex(VkPhysicalDevice physicalDevice, uint32_t memoryTypeBits, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if (memoryTypeBits & (1 << i)
			&& memProperties.memoryTypes[i].propertyFlags & properties)
		{
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type");
}

void createBufferAndMemory(Base base, VkDeviceSize size, VkBuffer* buffer, VkDeviceMemory* memory, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, NULL};
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.queueFamilyIndexCount = 1;
	bufferInfo.pQueueFamilyIndices = &base.queue.graphicsPresentFamilyIndex;
	bufferInfo.flags = 0;

	if (vkCreateBuffer(base.device, &bufferInfo, nullptr, buffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create buffer of size " + std::to_string(size));

	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(base.device, *buffer, &memReq);

	createMemory(base.device, base.physicalDevice, memReq.size, memory, properties, memReq.memoryTypeBits);
}

void createMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkDeviceMemory* memory, VkMemoryPropertyFlags properties, uint32_t memoryTypeBits)
{
	VkMemoryAllocateInfo memInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memInfo.allocationSize = size;
	memInfo.memoryTypeIndex = findMemoryIndex(physicalDevice, memoryTypeBits, properties);

	if (vkAllocateMemory(device, &memInfo, nullptr, memory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate memory of size " + std::to_string(size));
}

VkDeviceSize correctAlignment(VkDeviceSize desiredSize, VkDeviceSize alignment)
{
	if (desiredSize % alignment != 0)
		desiredSize = desiredSize + alignment - (desiredSize % alignment);
	return desiredSize;
}

VkSampler createTextureSampler(VkDevice device, VkPhysicalDevice physicalDevice, float maxLod, bool enableAnisotrophy)
{
  VkSampler sampler;
  
  VkPhysicalDeviceProperties deviceProps{};
  vkGetPhysicalDeviceProperties(physicalDevice, &deviceProps);
  VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = samplerInfo.addressModeU;
  samplerInfo.addressModeW = samplerInfo.addressModeU;
  if (settings::PIXELATED)
  {
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
  }
  else
  {
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
  }
  if(enableAnisotrophy)
  {
    samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = deviceProps.limits.maxSamplerAnisotropy;
  }
  else
  {
    samplerInfo.maxAnisotropy = 1.0f;
  }
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.maxLod = maxLod;
  samplerInfo.minLod = 0.0f;
  if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed create sampler");

  return sampler;
}

}//namespace end
