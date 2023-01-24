#include "vulkan_manager.h"
#include "parts/primary.h"
#include <vulkan/vulkan_core.h>

#include <iostream>
#include <stdexcept>

#define throwOnErr(result_expr, error_message) \
    if(result_expr != VK_SUCCESS) throw std::runtime_error(error_message);

VulkanManager::VulkanManager(GLFWwindow *window) {
    throwOnErr(
	part::create::Instance(&instance),
	"Failed to create Vulkan Instance");
#ifndef NDEBUG
    throwOnErr(
	 part::create::DebugMessenger(instance, &debugMessenger),
	 "Failed to create Debug Messenger");
#endif
    throwOnErr(
	 glfwCreateWindowSurface(instance, window, nullptr, &windowSurface),
	 "Failed to get Window Surface From GLFW");

    
	
}
