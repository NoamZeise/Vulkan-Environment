#ifndef VKHELPER_H
#define VKHELPER_H

#include <stdexcept>
#include <string>

#include "render_structs.h"
#include "descriptor_structs.h"
#include "pipeline.h"
#include "vulkan/vulkan_core.h"
#include "config.h"

namespace vkhelper
{
  uint32_t findMemoryIndex(VkPhysicalDevice physicalDevice, uint32_t memoryTypeBits, VkMemoryPropertyFlags properties);
  void createBufferAndMemory(Base base, VkDeviceSize size, VkBuffer* buffer, VkDeviceMemory* memory, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
  void createMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkDeviceMemory* memory, VkMemoryPropertyFlags properties, uint32_t memoryTypeBits);
  VkDeviceSize correctAlignment(VkDeviceSize desiredSize, VkDeviceSize alignment);
  VkSampler createTextureSampler(VkDevice device, VkPhysicalDevice physicalDevice, float maxLod, bool enableAnisotrophy);
  VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features);
  VkSampleCountFlagBits getMaxSupportedMsaaSamples(VkDevice device, VkPhysicalDevice physicalDevice);
};




#endif
