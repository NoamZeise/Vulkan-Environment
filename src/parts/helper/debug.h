#ifndef PART_HELPER_DEBUG_H
#define PART_HELPER_DEBUG_H

#include <volk.h>

#ifndef NDEBUG

void populateDebugMessengerCreateInfo(
	VkDebugUtilsMessengerCreateInfoEXT *createInfo,
	bool errorOnly);

VkResult createDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger);

#endif

#endif
