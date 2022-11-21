#ifndef PARTS_PRIMARY_H
#define PARTS_PRIMARY_H

#include "../render_structs.h"
#include "../config.h"

#include <stdexcept>
#include <array>
#include <set>
#include <cstring>
#include <iostream>


namespace part
{
    namespace create
    {
            void Instance(VkInstance* instance);
            void Device(VkInstance instance, Base* base, VkSurfaceKHR surface);
    #ifndef NDEBUG
            void DebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT* messenger);
    #endif
    }

#ifndef NDEBUG
    namespace destroy
    {
        void DebugMessenger(VkInstance instance,VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    }
#endif
}

#endif
