#ifndef PARTS_SWAPCHAIN_H
#define PARTS_SWAPCHAIN_H

#include "../volk.h"
#include <GLFW/glfw3.h>

#include <config.h>
#include <stdexcept>
#include <vector>

namespace part
{
    namespace create
    {
        std::vector<VkImage> Swapchain(VkDevice device, VkPhysicalDevice physicalDevice,
                                       VkSurfaceKHR surface, VkSwapchainKHR *swapchain,
                                       VkSurfaceFormatKHR *format, VkExtent2D *extent,
                                       GLFWwindow* window, bool vsync);
    }
}

#endif
