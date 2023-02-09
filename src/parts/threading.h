#ifndef PART_CREATE_THREADING_H
#define PART_CREATE_THREADING_H

#include <volk.h>

namespace part {
    namespace create {
      VkResult Semaphore(VkDevice device, VkSemaphore *semaphore);
      VkResult Fence(VkDevice device, VkFence *fence, bool startSignalled);
    }
}

#endif
