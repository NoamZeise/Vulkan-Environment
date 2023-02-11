#ifndef VKHELPER_H
#define VKHELPER_H


#include "render_structs/device_state.h"

#include <volk.h>

#include <vector>

namespace vkhelper
{
  uint32_t findMemoryIndex(VkPhysicalDevice physicalDevice,
			   uint32_t memoryTypeBits, VkMemoryPropertyFlags properties);
  VkResult createBufferAndMemory(DeviceState base, VkDeviceSize size,
				 VkBuffer* buffer, VkDeviceMemory* memory,
				 VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
  VkResult createMemory(VkDevice device, VkPhysicalDevice physicalDevice,
			VkDeviceSize size, VkDeviceMemory* memory,
			VkMemoryPropertyFlags properties, uint32_t memoryTypeBits);
  VkDeviceSize correctAlignment(VkDeviceSize desiredSize, VkDeviceSize alignment);
  VkSampler createTextureSampler(VkDevice device, VkPhysicalDevice physicalDevice,
				 float maxLod, bool enableAnisotrophy,
				 VkSamplerAddressMode addressMode);
  VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice,
			       const std::vector<VkFormat>& formats, VkImageTiling tiling,
			       VkFormatFeatureFlags features);
  VkSampleCountFlagBits getMaxSupportedMsaaSamples(VkDevice device, VkPhysicalDevice physicalDevice);
};




#endif
