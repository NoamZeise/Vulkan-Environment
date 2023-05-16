#ifndef PARTS_CORE_H
#define PARTS_CORE_H

#include <volk.h>
#include "../render_structs/device_state.h"

namespace part {
    namespace create {
	VkResult Instance(VkInstance* instance);
	
	VkResult Device(VkInstance instance,
			DeviceState* base,
			VkSurfaceKHR surface,
			EnabledFeatures requestFeatures);
    }
#ifndef NDEBUG
    namespace create {
	VkResult DebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT* messenger);
    }
    namespace destroy {
        void DebugMessenger(VkInstance instance,
			    VkDebugUtilsMessengerEXT debugMessenger,
			    const VkAllocationCallbacks* pAllocator);
    }
#endif
}
#endif
