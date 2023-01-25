#include "choose_physical_device.h"


#include <iostream>
#include "../part_macros.h"
#include "device.h"

bool physicalDeviceSuitable(VkPhysicalDevice candidateDevice,
				  VkSurfaceKHR surface,
				  const std::vector<VkQueueFamilyProperties> &queueFamilyProps,
				     uint32_t *graphicsPresentQueueID,
				     const std::vector<const char*> &requestedExtensions) {
    VkBool32 graphicQueueSupport = VK_FALSE;
    VkBool32 presentQueueSupport = VK_FALSE;
    uint32_t graphicsPresent;
    for (uint32_t j = 0; j < queueFamilyProps.size(); j++) {
	vkGetPhysicalDeviceSurfaceSupportKHR(candidateDevice, j, surface, &presentQueueSupport);
	if (requestedExtensionsSupported(candidateDevice, requestedExtensions) &&
	    queueFamilyProps[j].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentQueueSupport) {
	    graphicQueueSupport = VK_TRUE;
	    graphicsPresent = j;
	}
    }
    *graphicsPresentQueueID = graphicsPresent;
    return graphicQueueSupport && presentQueueSupport;
}

    
VkResult choosePhysicalDevice(VkInstance instance,
			      VkSurfaceKHR surface,
			      VkPhysicalDevice *physicalDevice,
			      uint32_t *graphicsPresentQueueFamilyId,
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
	if (physicalDeviceSuitable(gpus[i], surface, queueFamilies,
				   graphicsPresentQueueFamilyId, requestedExtensions)) {
	    *physicalDevice = gpus[i];
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
