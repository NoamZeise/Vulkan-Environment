#include "vulkan_manager.h"
#include "parts/core.h"
#include "parts/command.h"

#include <iostream>
#include <stdexcept>


#define throwOnErr(result_expr, error_message)                                 \
  if (result_expr != VK_SUCCESS)                                               \
    throw std::runtime_error(error_message);

/*VkResult loadVulkan() {
    VkResult result = volkInitialize();
    if(result != VK_SUCCESS) {
	return result;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    return result;
    }*/

VulkanManager::VulkanManager(GLFWwindow *window, EnabledFeatures featuresToEnable) {
    this->window = window;
    // throwOnErr(loadVulkan(),
    //	       "Failed to load Vulkan functions");
    throwOnErr(part::create::Instance(&instance),
	       "Failed to create Vulkan Instance");
#ifndef NDEBUG
    throwOnErr(part::create::DebugMessenger(instance, &debugMessenger),
	       "Failed to create Debug Messenger");
#endif
    throwOnErr(glfwCreateWindowSurface(instance, window, nullptr, &windowSurface),
	       "Failed to get Window Surface From GLFW");
    throwOnErr(part::create::Device(instance, &deviceState, windowSurface, featuresToEnable),
	       "Failed to get physical device and create logical device");
    throwOnErr(part::create::CommandPoolAndBuffer(
		       deviceState.device,
		       &generalCommandPool,
		       &generalCommandBuffer,
		       deviceState.queue.graphicsPresentFamilyIndex),
	       "Failed to create command pool and buffer");
}


VulkanManager::~VulkanManager() {
    vkQueueWaitIdle(deviceState.queue.graphicsPresentQueue);

    
    vkDestroyCommandPool(deviceState.device, generalCommandPool, nullptr);
    vkDestroyDevice(deviceState.device, nullptr);
    vkDestroySurfaceKHR(instance, windowSurface, nullptr);
#ifndef NDEBUG
    part::destroy::DebugMessenger(instance, debugMessenger, nullptr);
#endif
    vkDestroyInstance(instance, nullptr);
}
