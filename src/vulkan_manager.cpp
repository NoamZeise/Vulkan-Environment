#include "vulkan_manager.h"
#include "parts/core.h"
#include "parts/command.h"

#include <iostream>
#include <stdexcept>

std::mutex graphicsPresentMutex;

#define throwOnErr(result_expr, error_message)                                 \
  if (result_expr != VK_SUCCESS)                                               \
    throw std::runtime_error(error_message);

VulkanManager::VulkanManager(GLFWwindow *window, EnabledFeatures featuresToEnable) {
    this->window = window;
    throwOnErr(part::create::Instance(&instance),
	       "Failed to create Vulkan Instance");
#ifndef NDEBUG
    throwOnErr(part::create::DebugMessenger(instance, &debugMessenger,
					    featuresToEnable.debugErrorOnly),
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
		       deviceState.queue.graphicsPresentFamilyIndex, 0),
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
