#include "threading.h"


namespace part {
  namespace create {
    VkResult Semaphore(VkDevice device, VkSemaphore *semaphore) {
	return Semaphore(device, semaphore, nullptr);
    }

    VkResult Semaphore(VkDevice device, VkSemaphore *semaphore, void* pNext) {
	VkSemaphoreCreateInfo semaphoreInfo{
	    VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	semaphoreInfo.pNext = pNext;
	return vkCreateSemaphore(device, &semaphoreInfo, nullptr, semaphore);
    }

    VkResult TimelineSemaphore(VkDevice device, VkSemaphore *semaphore) {
	VkSemaphoreTypeCreateInfo t{VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
	t.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	t.initialValue = 0;
	return Semaphore(device, semaphore, &t);
    }

    VkResult Fence(VkDevice device, VkFence *fence, bool startSignalled) {
	VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
	if(startSignalled) fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	else fenceInfo.flags = 0;
	return vkCreateFence(device, &fenceInfo, nullptr, fence);
    }
  }
}
