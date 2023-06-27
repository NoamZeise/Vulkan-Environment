#include "command.h"

#include <iostream>

#include "../logger.h"

namespace part {
  namespace create {
      VkResult CommandPoolAndBuffer(VkDevice device,
				    VkCommandPool *commandPool,
				    VkCommandBuffer *commandBuffer,
				    uint32_t queueFamilyIndex,
				    VkCommandPoolCreateFlags flags) {
      VkResult result = VK_SUCCESS;
      VkCommandPoolCreateInfo commandPoolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
      commandPoolInfo.queueFamilyIndex = queueFamilyIndex;
      commandPoolInfo.flags = flags;
      msgAndReturnOnErr(vkCreateCommandPool(device, &commandPoolInfo, nullptr, commandPool),
			"Failed to create command pool");
      
      VkCommandBufferAllocateInfo commandBufferInfo{
	VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
      commandBufferInfo.commandPool = *commandPool;
      commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      commandBufferInfo.commandBufferCount = 1;
      msgAndReturnOnErr(vkAllocateCommandBuffers(device, &commandBufferInfo, commandBuffer),
			"Failed to allocate command buffer");

      return result;
    }
  

} // namspace create  
    
    
  } // namespace parts
  
