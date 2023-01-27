#ifndef VULKAN_MANAGER_H
#define VULKAN_MANAGER_H

#include <volk.h>
#include <GLFW/glfw3.h>

#include "render_structs.h"

class VulkanManager {
public:
    VulkanManager(GLFWwindow* window);
    ~VulkanManager();
    VkResult initFrameResources();
    void destroyFrameResources();
private:
    VkInstance instance;
    VkSurfaceKHR windowSurface;
    DeviceState deviceState;
    VkCommandPool generalCommandPool;
    VkCommandBuffer generalCommandBuffer;
    SwapChain swapchain;

    VkRenderPass primaryRenderPass;
    VkRenderPass finalRenderPass;

    // frame state
    uint32_t frameIndex;
    VkSemaphore imageAquiredSem;
  
#ifndef NDEBUG
    VkDebugUtilsMessengerEXT debugMessenger;
#endif
};

#endif
