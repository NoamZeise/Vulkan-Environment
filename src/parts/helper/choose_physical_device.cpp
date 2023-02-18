#include "choose_physical_device.h"


#include <iostream>
#include <memory>

#include "../part_macros.h"
#include "device.h"

#include "../../logger.h"


bool graphicsPresentSupported(VkPhysicalDevice candidate, VkQueueFamilyProperties queueProps, uint32_t queueId, VkSurfaceKHR surface) {
    VkBool32 presentQueueSupported;
    vkGetPhysicalDeviceSurfaceSupportKHR(candidate, queueId, surface, &presentQueueSupported);
    return presentQueueSupported && queueProps.queueFlags & VK_QUEUE_GRAPHICS_BIT;
}

bool getGraphicsPresentQueueID(VkPhysicalDevice candidateDevice,
			       VkSurfaceKHR surface,
			       VkQueueFamilyProperties* queueFamilyProps,
			       uint32_t queueFamilySize,
			       uint32_t *pGraphicsPresentQueueID,
			       const std::vector<const char*> &requestedExtensions) {
    for (uint32_t j = 0; j < queueFamilySize; j++) {
	if(checkRequestedExtensionsAreSupported(candidateDevice, requestedExtensions) &&
	   graphicsPresentSupported(candidateDevice, queueFamilyProps[j], j, surface))  {
	    *pGraphicsPresentQueueID = j;
	    return true;
	}
    }
    return false;
}

    
VkResult choosePhysicalDevice(VkInstance instance,
			      VkSurfaceKHR surface,
			      VkPhysicalDevice *pPhysicalDevice,
			      uint32_t *pGraphicsPresentQueueFamilyId,
			      const std::vector<const char*> &requestedExtensions) {
    VkResult result;
    uint32_t deviceCount;
    msgAndReturnOnErr(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr),
		      "Failed to get device count!");

    if (deviceCount == 0) {
	std::cerr << "Error: No Physical Devices Found\n";
	return VK_ERROR_INITIALIZATION_FAILED;
    }

    std::unique_ptr<VkPhysicalDevice> gpus(new VkPhysicalDevice[deviceCount]);
    msgAndReturnOnErr(vkEnumeratePhysicalDevices(instance, &deviceCount, gpus.get()),
		      "Failed to get device list!");

    VkPhysicalDeviceProperties bestDeviceProperties;
    bool foundSuitable = false;
    for (size_t i = 0; i < deviceCount; i++) {

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(gpus.get()[i], &deviceProperties);

	LOG("Candidate Physical Device Name: " << deviceProperties.deviceName); 
	
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(gpus.get()[i], &queueFamilyCount, nullptr);
	std::unique_ptr<VkQueueFamilyProperties>
	    queueFamilies(new VkQueueFamilyProperties[queueFamilyCount]);
	vkGetPhysicalDeviceQueueFamilyProperties(gpus.get()[i], &queueFamilyCount,
						 queueFamilies.get());
	if (getGraphicsPresentQueueID(gpus.get()[i], surface, queueFamilies.get(), queueFamilyCount,
				      pGraphicsPresentQueueFamilyId, requestedExtensions)) {
	    *pPhysicalDevice = gpus.get()[i];
	    foundSuitable = true;
	    bestDeviceProperties = deviceProperties;
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

    LOG("Candidate chosen: " << bestDeviceProperties.deviceName);
    
    return result;
}
