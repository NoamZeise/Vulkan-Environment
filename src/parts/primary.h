#ifndef PARTS_PRIMARY_H
#define PARTS_PRIMARY_H

#include <volk.h>
#include <vulkan/vulkan_core.h>
#include "../render_structs.h"


namespace part
{
    namespace create
    {
	VkResult Instance(VkInstance* instance);
	VkResult Device(VkInstance instance,
			DeviceState* base,
			VkSurfaceKHR surface,
			EnabledFeatures requestFeatures);
    #ifndef NDEBUG
	VkResult DebugMessenger(VkInstance instance,
				VkDebugUtilsMessengerEXT* messenger);
    #endif
    }

#ifndef NDEBUG
    namespace destroy
    {
        void DebugMessenger(VkInstance instance,
			    VkDebugUtilsMessengerEXT debugMessenger,
			    const VkAllocationCallbacks* pAllocator);
    }
#endif
}

#endif
