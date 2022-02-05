#include "vkhelper.h"

namespace vkhelper 
{

uint32_t findMemoryIndex(VkPhysicalDevice physicalDevice, uint32_t memoryTypeBits, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	for (size_t i = 0; i < memProperties.memoryTypeCount; i++)
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
	VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.queueFamilyIndexCount = 1;
	bufferInfo.pQueueFamilyIndices = &base.queue.graphicsPresentFamilyIndex;

	if (vkCreateBuffer(base.device, &bufferInfo, nullptr, buffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create buffer of size " + size);

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
		throw std::runtime_error("failed to allocate memory of size " + size);
}


}//namespace end
