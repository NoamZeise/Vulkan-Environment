#ifndef PARTS_COMMAND_H
#define PARTS_COMMAND_H

#include <volk.h>

namespace part {
  namespace create {
     VkResult CommandPoolAndBuffer(VkDevice device,
				    VkCommandPool *commandPool,
				    VkCommandBuffer *commandBuffer,
				   uint32_t queueFamilyIndex);
  }
}

#endif
