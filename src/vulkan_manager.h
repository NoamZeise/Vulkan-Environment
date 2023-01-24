#ifndef VULKAN_MANAGER_H
#define VULKAN_MANAGER_H

#include <volk.h>
#include <GLFW/glfw3.h>

#include "render_structs.h"

class VulkanManager {
 public:
  VulkanManager(GLFWwindow* window);

 private:
  VkInstance instance;
  VkSurfaceKHR windowSurface;
  DeviceState deviceState;
  SwapChain swapchain;

  VkRenderPass primaryRenderPass;
  VkRenderPass finalRenderPass;

  VkCommandPool dataTransferCommandPool;
  VkCommandBuffer dataTransferCommandBuffer;

  // frame state
  uint32_t frameIndex;
  VkSemaphore imageAquiredSem;
  
#ifndef NDEBUG
  VkDebugUtilsMessengerEXT debugMessenger;
#endif
};

#endif
