#ifndef VULKAN_MANAGER_H
#define VULKAN_MANAGER_H

#include <stdint.h>

#include <volk.h>
#include <GLFW/glfw3.h>

#include "render_structs/device_state.h"
#include "render_structs/swapchain/swapchain.h"

class VulkanManager {
public:
    VulkanManager(GLFWwindow *window, EnabledFeatures featuresToEnable);
    ~VulkanManager();
    VkResult initFrameResources();
    void destroyFrameResources();
    void StartDraw();
    void EndDraw();
private:
    bool useWindowResolution = true;
    VkExtent2D offscreenExtent = {0, 0};
    bool vsync = true;
    bool srgb = false;
    bool multisampling = true;
    
    GLFWwindow *window;
    VkInstance instance;
    VkSurfaceKHR windowSurface;
    DeviceState deviceState;
    VkCommandPool generalCommandPool;
    VkCommandBuffer generalCommandBuffer;
    Swapchain* swapchain;

    bool frameResourcesCreated = false;
  
#ifndef NDEBUG
    VkDebugUtilsMessengerEXT debugMessenger;
#endif
};

#endif
