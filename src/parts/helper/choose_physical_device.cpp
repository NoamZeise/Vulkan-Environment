#include "choose_physical_device.h"


#include <iostream>
#include "../part_macros.h"
#include "device.h"

bool getGraphicsPresentQueueID(VkPhysicalDevice candidateDevice,
			       VkSurfaceKHR surface,
			       const std::vector<VkQueueFamilyProperties> &queueFamilyProps,
			       uint32_t *pGraphicsPresentQueueID,
			       const std::vector<const char*> &requestedExtensions);

    
VkResult choosePhysicalDevice(VkInstance instance,
			      VkSurfaceKHR surface,
			      VkPhysicalDevice *pPhysicalDevice,
			      uint32_t *pGraphicsPresentQueueFamilyId,
			      const std::vector<const char*> &requestedExtensions) {
    VkResult result;
    uint32_t deviceCount;
    msgAndReturnOnErr(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr),
		      "Failed to get device count!");

    if (deviceCount < 1) {
	std::cerr << "Error: No Physical Devices Found\n";
	return VK_ERROR_INITIALIZATION_FAILED;
    }

    std::vector<VkPhysicalDevice> gpus(deviceCount);
    msgAndReturnOnErr(vkEnumeratePhysicalDevices(instance, &deviceCount, gpus.data()),
		      "Failed to get device list!");

    bool foundSuitable = false;
    for (size_t i = 0; i < gpus.size(); i++) {

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(gpus[i], &deviceProperties);
      
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(gpus[i], &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(gpus[i], &queueFamilyCount, queueFamilies.data());
	if (getGraphicsPresentQueueID(gpus[i], surface, queueFamilies, pGraphicsPresentQueueFamilyId, requestedExtensions)) {
	    *pPhysicalDevice = gpus[i];
	    foundSuitable = true;
	    // prefer discreet GPUs
	    if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		break;
	}
    }
    if (!foundSuitable)  {
	std::cerr << "Error: Failed to find device which supports graphics and present queues, "
	    "as well as the requested extensions\n";
	return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    return result;
}



bool graphicsPresentSupported(VkPhysicalDevice candidate, VkQueueFamilyProperties queueProps, uint32_t queueId, VkSurfaceKHR surface) {
    VkBool32 presentQueueSupported;
    vkGetPhysicalDeviceSurfaceSupportKHR(candidate, queueId, surface, &presentQueueSupported);
    return presentQueueSupported && queueProps.queueFlags & VK_QUEUE_GRAPHICS_BIT;
}

bool getGraphicsPresentQueueID(VkPhysicalDevice candidateDevice,
				VkSurfaceKHR surface,
				const std::vector<VkQueueFamilyProperties> &queueFamilyProps,
				uint32_t *pGraphicsPresentQueueID,
				const std::vector<const char*> &requestedExtensions) {
    for (uint32_t j = 0; j < queueFamilyProps.size(); j++) {
	if(checkRequestedExtensionsAreSupported(candidateDevice, requestedExtensions) &&
	   graphicsPresentSupported(candidateDevice, queueFamilyProps[j], j, surface))  {
	    *pGraphicsPresentQueueID = j;
	    return true;
	}
    }
    return false;
}
