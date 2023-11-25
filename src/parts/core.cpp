#include  "core.h"
#include <GLFW/glfw3.h>

#include <array>
#include <set>
#include <cstring>
#include <iostream>
#include <stdint.h>
#include <vector>

#include "../logger.h"
#ifndef NDEBUG
#include "helper/debug.h"
#endif
#include "helper/device.h"
#include "helper/choose_physical_device.h"

namespace part {
    
  namespace create {
    const std::vector<const char *> OPTIONAL_LAYERS = {
#ifndef NDEBUG
	"VK_LAYER_KHRONOS_validation",
#else
	
#endif
    };
    
    const std::vector<VkValidationFeatureEnableEXT> VALIDATION_LAYER_FEATURES = {
#ifndef NDEBUG
	VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
	VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
#else
    
#endif
    };

    const std::vector<const char*> REQUESTED_DEVICE_EXTENSIONS = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	
    
    VkResult Instance(VkInstance *instance) {
	VkResult result = VK_SUCCESS;
	VkInstanceCreateInfo instanceCreateInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };

	// app info
	VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
	appInfo.pApplicationName = "Vulkan App";
	appInfo.pEngineName = "No Engine";
	appInfo.apiVersion = VK_API_VERSION_1_0;
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	instanceCreateInfo.pApplicationInfo = &appInfo;

	// check required extensions are supported
	uint32_t requiredExtensionsCount = 0;
	const char **requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionsCount);
	std::vector<const char *> extensions(requiredExtensions, requiredExtensions + requiredExtensionsCount);
#ifndef NDEBUG
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

	if (!checkRequiredLayersSupported(OPTIONAL_LAYERS)) {
	    std::cerr << "ERROR: extension layers that were requested aren't "
		"supported by the chosen device. Make sure you have the Vulkan SDK installed, and "
		"that the requested layers are available.\n";
	    return VK_ERROR_FEATURE_NOT_PRESENT;
	}
	
#ifndef NDEBUG
	instanceCreateInfo.enabledLayerCount =  static_cast<uint32_t>(OPTIONAL_LAYERS.size());
	instanceCreateInfo.ppEnabledLayerNames = OPTIONAL_LAYERS.data();

	VkValidationFeaturesEXT validationFeatures{ VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
	validationFeatures.enabledValidationFeatureCount = static_cast<uint32_t>(VALIDATION_LAYER_FEATURES.size());
	validationFeatures.pEnabledValidationFeatures = VALIDATION_LAYER_FEATURES.data();
	instanceCreateInfo.pNext = &validationFeatures; // call after create instance

	// setup debug messenger for just the creation of an instance
	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
	populateDebugMessengerCreateInfo(&debugMessengerInfo, false);
	validationFeatures.pNext = &debugMessengerInfo; // call after validation feature creation
#else
	instanceCreateInfo.enabledLayerCount = 0;
	instanceCreateInfo.pNext = nullptr;
#endif

	returnOnErr(vkCreateInstance(&instanceCreateInfo, nullptr, instance));
	volkLoadInstance(*instance);
	return VK_SUCCESS;
    }


    VkResult Device(VkInstance instance,
		    DeviceState *deviceState,
		    VkSurfaceKHR surface,
		    EnabledFeatures requestFeatures) {
	VkResult result = VK_SUCCESS;
	// get a suitable physical device
	returnOnErr(choosePhysicalDevice(
			    instance,
			    surface,
			    &deviceState->physicalDevice,
			    &deviceState->queue.graphicsPresentFamilyIndex,
			    REQUESTED_DEVICE_EXTENSIONS));
	// create logical device
	VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};

	float queuePriority = 1.0f;
	std::vector<VkDeviceQueueCreateInfo> queueInfos = fillQueueFamiliesCreateInfo(
		{ deviceState->queue.graphicsPresentFamilyIndex }, &queuePriority);
	    
	deviceInfo.queueCreateInfoCount = (uint32_t)queueInfos.size();
	deviceInfo.pQueueCreateInfos = queueInfos.data();

	deviceInfo.enabledExtensionCount = (uint32_t)REQUESTED_DEVICE_EXTENSIONS.size();
	deviceInfo.ppEnabledExtensionNames = REQUESTED_DEVICE_EXTENSIONS.data();
	    
	VkPhysicalDeviceFeatures chosenDeviceFeatures = setRequestedDeviceFeatures(
		deviceState->physicalDevice,
		requestFeatures,
		&deviceState->features);
	deviceInfo.pEnabledFeatures = &chosenDeviceFeatures;

	deviceInfo.enabledLayerCount = (uint32_t)OPTIONAL_LAYERS.size();
	deviceInfo.ppEnabledLayerNames = OPTIONAL_LAYERS.data();

	returnOnErr(vkCreateDevice(deviceState->physicalDevice, &deviceInfo, nullptr,
				   &deviceState->device));

	volkLoadDevice(deviceState->device);
	// get queue handles for graphics and present
	vkGetDeviceQueue(deviceState->device,
			 deviceState->queue.graphicsPresentFamilyIndex, 0,
			 &deviceState->queue.graphicsPresentQueue);
	return result;
    }

    
#ifndef NDEBUG
    VkResult DebugMessenger(VkInstance instance,
			    VkDebugUtilsMessengerEXT *messenger,
			    bool errorOnly) {
	VkResult result = VK_SUCCESS;
	// setup debug messenger for all operations
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	populateDebugMessengerCreateInfo(&createInfo, errorOnly);
	returnOnErr(createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, messenger));
	return result;
    }
#endif

  } // namespace create

#ifndef NDEBUG
  namespace destroy {

    void DebugMessenger(VkInstance instance,
			VkDebugUtilsMessengerEXT debugMessenger,
			const VkAllocationCallbacks *pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
	    func(instance, debugMessenger, pAllocator);
	} else {
	    std::cerr << "Error: failed to proc address of destroy debug utils fn\n";
	}
    }

  } // namespace destroy
#endif

} // namespace part
