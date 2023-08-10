#include "command.h"

#include <iostream>

#include "../logger.h"

namespace part {
  namespace create {
    VkResult CommandBuffer(VkDevice device, VkCommandPool pool, VkCommandBuffer *cmdbuff) {
	VkCommandBufferAllocateInfo commandBufferInfo{
	    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	commandBufferInfo.commandPool = pool;
	commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferInfo.commandBufferCount = 1;
	return vkAllocateCommandBuffers(device, &commandBufferInfo, cmdbuff);
    }
    
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
	
	msgAndReturnOnErr(CommandBuffer(device, *commandPool, commandBuffer),
			  "Failed to allocate command buffer");
	
	return result;
    }

} // namspace create    
} // namespace parts
  
