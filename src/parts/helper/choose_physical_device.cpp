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

std::string getPhysicalDeviceTypeStr(VkPhysicalDeviceType type);

bool rankPhysicalDevices(VkPhysicalDeviceProperties bestProps,
			 VkPhysicalDeviceProperties currentProps);
    
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
	std::string deviceType = getPhysicalDeviceTypeStr(deviceProperties.deviceType);
	LOG("Type: " << deviceType);
	
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(gpus.get()[i], &queueFamilyCount, nullptr);
	std::unique_ptr<VkQueueFamilyProperties>
	    queueFamilies(new VkQueueFamilyProperties[queueFamilyCount]);
	vkGetPhysicalDeviceQueueFamilyProperties(gpus.get()[i], &queueFamilyCount,
						 queueFamilies.get());
	if (getGraphicsPresentQueueID(gpus.get()[i], surface, queueFamilies.get(), queueFamilyCount,
				      pGraphicsPresentQueueFamilyId, requestedExtensions)) {
	    
	    if(!foundSuitable || rankPhysicalDevices(bestDeviceProperties, deviceProperties)) {
		*pPhysicalDevice = gpus.get()[i];
		foundSuitable = true;
		bestDeviceProperties = deviceProperties;
	    }
	}
    }
    if (!foundSuitable)  {
	LOG_ERROR("Error: Failed to find device which supports graphics and present queues, "
		  "as well as the requested extensions");
	return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    LOG("Candidate chosen: " << bestDeviceProperties.deviceName);
    
    return result;
}


std::string getPhysicalDeviceTypeStr(VkPhysicalDeviceType type) {
    if(type == VK_PHYSICAL_DEVICE_TYPE_CPU)
	return "CPU";
    if(type == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
	return "Integrated GPU";
    if(type == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	return "Discrete GPU";
    if(type == VK_PHYSICAL_DEVICE_TYPE_OTHER)
	return "Other";
    if(type == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
	return "Virtual GPU";
    if(type == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
	return "IntegrateGPU";
    return "None";
}

const size_t DEVICE_RANKING_COUNT = 6;
const VkPhysicalDeviceType DEVICE_TYPE_RANKINGS[DEVICE_RANKING_COUNT] = {
    VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
    VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
    VK_PHYSICAL_DEVICE_TYPE_CPU,
    VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
    VK_PHYSICAL_DEVICE_TYPE_OTHER,
};

// lower is better
size_t typeRank(VkPhysicalDeviceProperties bestProps) { 
    for(size_t i = 0; i < DEVICE_RANKING_COUNT; i++) 
	if(DEVICE_TYPE_RANKINGS[i] == bestProps.deviceType)
	    return i;
    return DEVICE_RANKING_COUNT;
}

bool rankPhysicalDevices(VkPhysicalDeviceProperties bestProps,
			 VkPhysicalDeviceProperties currentProps) {
    if(typeRank(currentProps) < typeRank(bestProps)) {
	return true;
    }
    
    return false;
}
