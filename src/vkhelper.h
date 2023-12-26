#ifndef VKHELPER_H
#define VKHELPER_H

#include "device_state.h"
#include <volk.h>
#include <vector>
#include <mutex>

namespace vkhelper {
  /// find a memory index that has the desired type and supports the required properties
  uint32_t findMemoryIndex(VkPhysicalDevice physicalDevice,
			   uint32_t memoryTypeBits, VkMemoryPropertyFlags properties);
  
  VkResult createBufferAndMemory(DeviceState base, VkDeviceSize size,
				 VkBuffer* buffer, VkDeviceMemory* memory,
				 VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
  
  VkResult allocateMemory(VkDevice device, VkPhysicalDevice physicalDevice,
			  VkDeviceSize size, VkDeviceMemory* memory,
			  VkMemoryPropertyFlags properties, uint32_t memoryTypeBits);

  /// return the desired size padded to match the required alignment
  VkDeviceSize correctMemoryAlignment(VkDeviceSize desiredSize, VkDeviceSize alignment);
  
  VkSampler createTextureSampler(VkDevice device, VkPhysicalDevice physicalDevice,
				 float maxLod, bool enableAnisotrophy, bool useNearestFilter,
				 VkSamplerAddressMode addressMode);

  /// return a vector list of formats the hardware supports
  VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice,
			       const std::vector<VkFormat>& formats, VkImageTiling tiling,
			       VkFormatFeatureFlags features);
  
  VkSampleCountFlagBits getMaxSupportedMsaaSamples(VkDevice device,
						   VkPhysicalDevice physicalDevice);
  
  VkResult submitQueue(VkQueue queue, VkSubmitInfo* info, std::mutex* queueMut, VkFence fence);
  
  VkResult submitCmdBuffAndWait(VkDevice device, VkQueue queue,
				VkCommandBuffer* cmdbuff, VkFence fence,
				std::mutex* queueSubmitMutex);
};




#endif
