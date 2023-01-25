#include "vulkan_manager.h"
#include "config.h"
#include "parts/core.h"
#include <vulkan/vulkan_core.h>

#include <iostream>
#include <stdexcept>


#define throwOnErr(result_expr, error_message)                                 \
  if (result_expr != VK_SUCCESS)                                               \
    throw std::runtime_error(error_message);


VulkanManager::VulkanManager(GLFWwindow *window) {
    throwOnErr(part::create::Instance(&instance),
	       "Failed to create Vulkan Instance");
#ifndef NDEBUG
    throwOnErr(part::create::DebugMessenger(instance, &debugMessenger),
	       "Failed to create Debug Messenger");
#endif
    throwOnErr(glfwCreateWindowSurface(instance, window, nullptr, &windowSurface),
	       "Failed to get Window Surface From GLFW");
    
    throwOnErr(part::create::Device(instance, &deviceState, windowSurface, EnabledFeatures { true, settings::SAMPLE_SHADING }),
	       "Failed to get physical device and create logical device");
}


VulkanManager::~VulkanManager() {
    vkQueueWaitIdle(deviceState.queue.graphicsPresentQueue);
    
	
    vkDestroyDevice(deviceState.device, nullptr);
    vkDestroySurfaceKHR(instance, windowSurface, nullptr);
#ifndef NDEBUG
    part::destroy::DebugMessenger(instance, debugMessenger, nullptr);
#endif
    vkDestroyInstance(instance, nullptr);
}
