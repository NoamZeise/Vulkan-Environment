#ifndef PARTS_SWAPCHAIN_H
#define PARTS_SWAPCHAIN_H

#include "../render_structs.h"
#include "../config.h"
#include "../vkhelper.h"

#include <stdexcept>

namespace part
{
    namespace create
    {
        void Swapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, SwapChain* swapchain, GLFWwindow* window, uint32_t graphicsQueueIndex);
    }
    namespace destroy
    {
        void Swapchain(VkDevice device, SwapChain* swapchain);
    }
}

#endif
