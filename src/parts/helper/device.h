#ifndef PART_HELPER_DEVICE_H
#define PART_HELPER_DEVICE_H

#include <volk.h>
#include <vector>
#include <set>
#include <vulkan/vulkan_core.h>
#include "../../device_state.h"

bool checkRequiredLayersSupported(const std::vector<const char *> requiredLayers);

bool checkRequestedExtensionsAreSupported(
    VkPhysicalDevice physicalDevice,
    const std::vector<const char *> &requestedExtensions);

std::vector<VkDeviceQueueCreateInfo> fillQueueFamiliesCreateInfo(std::set<uint32_t> uniqueQueueFamilies, float *queuePriority);

VkPhysicalDeviceFeatures setRequestedDeviceFeatures(
	VkPhysicalDevice physicalDevice,
	EnabledFeatures requestedFeatures,
	EnabledFeatures*  setFeatures);
#endif
