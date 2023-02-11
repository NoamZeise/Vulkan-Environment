#ifndef VULKAN_MANAGER_H
#define VULKAN_MANAGER_H

#include <stdint.h>

#include <volk.h>
#include <GLFW/glfw3.h>

#include "render_structs/device_state.h"

struct VulkanManager {
    VulkanManager(GLFWwindow *window, EnabledFeatures featuresToEnable);
    ~VulkanManager();

    DeviceState deviceState;
    VkCommandPool generalCommandPool;
    VkCommandBuffer generalCommandBuffer;
    /*bool useWindowResolution = true;
    VkExtent2D offscreenExtent = {0, 0};
    bool vsync = true;
    bool srgb = false;
    bool multisampling = true;*/
    
    GLFWwindow *window;
    VkInstance instance;
    VkSurfaceKHR windowSurface;

  
#ifndef NDEBUG
    VkDebugUtilsMessengerEXT debugMessenger;
#endif
};

#endif
