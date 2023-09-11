/// Holds the per app vulkan resources.
/// i.e Graphics device, logical device, GFLW window, surface for rendering.
/// These resources are usually created and destroyed once per app.

#ifndef VULKAN_MANAGER_H
#define VULKAN_MANAGER_H

#include <stdint.h>

#include <volk.h>
#include <GLFW/glfw3.h>

#include "device_state.h"

struct VulkanManager {
    VulkanManager(GLFWwindow *window, EnabledFeatures featuresToEnable);
    ~VulkanManager();

    DeviceState deviceState;
    VkCommandPool generalCommandPool;
    VkCommandBuffer generalCommandBuffer;
    GLFWwindow *window;
    VkInstance instance;
    VkSurfaceKHR windowSurface;
#ifndef NDEBUG
    VkDebugUtilsMessengerEXT debugMessenger;
#endif
};

#endif
