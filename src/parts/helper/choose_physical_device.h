#ifndef PART_HELPER_CHOOSE_PHYSICAL_H
#define PART_HELPER_CHOOSE_PHYSICAL_H

#include <volk.h>
#include <vector>

VkResult choosePhysicalDevice(VkInstance instance, VkSurfaceKHR surface,
			      VkPhysicalDevice *physicalDevice,
			      uint32_t *graphicsPresentQueueFamilyId,
			      const std::vector<const char*> &requestedExtensions);

#endif
