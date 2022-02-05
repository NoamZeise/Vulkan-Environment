#ifndef VKHELPER_H
#define VKHELPER_H

#include <stdexcept>

#include "render_structs.h"
#include "descriptor_sets.h"
#include "pipeline.h"

namespace vkhelper
{
	uint32_t findMemoryIndex(VkPhysicalDevice physicalDevice, uint32_t memoryTypeBits, VkMemoryPropertyFlags properties);
	void createBufferAndMemory(Base base, VkDeviceSize size, VkBuffer* buffer, VkDeviceMemory* memory, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	void createMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkDeviceMemory* memory, VkMemoryPropertyFlags properties, uint32_t memoryTypeBits);
};




#endif
