#ifndef PART_CREATE_THREADING_H
#define PART_CREATE_THREADING_H

#include <volk.h>

namespace part {
    namespace create {
	VkResult Semaphore(VkDevice device, VkSemaphore *semaphore) {
	    VkSemaphoreCreateInfo semaphoreInfo{
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	    return vkCreateSemaphore(device, &semaphoreInfo, nullptr, semaphore);
	}

	VkResult Fence(VkDevice device, VkFence *fence, bool startSignalled) {
	    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
	    if(startSignalled) fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	    else fenceInfo.flags = 0;
	    return vkCreateFence(device, &fenceInfo, nullptr, fence);
	}
    }
}

#endif
