#ifndef PARTS_THREADING_H
#define PARTS_THREADING_H

#include <volk.h>

namespace part {
    namespace create {
      VkResult Semaphore(VkDevice device, VkSemaphore *semaphore);
      VkResult Semaphore(VkDevice device, VkSemaphore *semaphore, void* pNext);
      VkResult TimelineSemaphore(VkDevice device, VkSemaphore *semaphore);
      VkResult Fence(VkDevice device, VkFence *fence, bool startSignalled);
    }
}

#endif
