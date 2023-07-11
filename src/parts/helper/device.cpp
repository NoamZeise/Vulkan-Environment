#include "device.h"

#include "../../logger.h"
#include <iostream>
#include <cstring>

#define checkStringsAgainstList(str_list, other, path_to_str) \
    for (auto test_str : str_list) {  \
	bool found = false;  \
	for (auto e: other) {						\
	    if (std::strcmp(test_str, e.path_to_str) == 0) {\
		found = true;\
		break;\
	    }\
	}\
	if (!found) {\
	    return false;\
	}\
    }\
    return true;				\

bool checkRequiredLayersSupported(std::vector<const char *> requiredLayers) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    checkStringsAgainstList(requiredLayers, availableLayers, layerName);
}

bool checkRequestedExtensionsAreSupported(
	VkPhysicalDevice physicalDevice,
	const std::vector<const char *> &requestedExtensions) {
    uint32_t extensionCount;
    if(vkEnumerateDeviceExtensionProperties(
	       physicalDevice,
	       nullptr,
	       &extensionCount,
	       nullptr)  != VK_SUCCESS) {
	std::cerr <<  "Error: failed to find device extensions count\n";
	return  false;
    }
    std::vector<VkExtensionProperties> deviceExtensions(extensionCount);
    if(vkEnumerateDeviceExtensionProperties(
	       physicalDevice,
	       nullptr,
	       &extensionCount,
	       deviceExtensions.data()) != VK_SUCCESS) {
	std::cerr << "Error: failed to find device extenions\n";
    }

    checkStringsAgainstList(requestedExtensions, deviceExtensions, extensionName);
    return true;
}

std::vector<VkDeviceQueueCreateInfo> fillQueueFamiliesCreateInfo(std::set<uint32_t> uniqueQueueFamilies, float *queuePriority) {
    std::vector<VkDeviceQueueCreateInfo> queueInfos(uniqueQueueFamilies.size());
    int familyCount = 0;
    for (uint32_t familyIndex : uniqueQueueFamilies) {
	VkDeviceQueueCreateInfo queueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	queueCreateInfo.queueFamilyIndex = familyIndex;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = queuePriority;
   
	queueInfos[familyCount] = queueCreateInfo;
	familyCount++;
    }
    return queueInfos;
}

VkPhysicalDeviceFeatures setRequestedDeviceFeatures(
	VkPhysicalDevice physicalDevice,
	EnabledFeatures requestedFeatures, EnabledFeatures *setFeatures) {
    VkPhysicalDeviceFeatures availableDeviceFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &availableDeviceFeatures);
    VkPhysicalDeviceFeatures chosenDeviceFeatures{};
    if (availableDeviceFeatures.samplerAnisotropy && requestedFeatures.samplerAnisotropy) {
	chosenDeviceFeatures.samplerAnisotropy = VK_TRUE;
	setFeatures->samplerAnisotropy = true;
    }
    if (availableDeviceFeatures.sampleRateShading && requestedFeatures.sampleRateShading) {
	chosenDeviceFeatures.sampleRateShading = VK_TRUE;
	setFeatures->sampleRateShading = true;
    }
    return chosenDeviceFeatures;
}

