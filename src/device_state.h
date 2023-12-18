#ifndef RENDER_STRUCTS_DEVICE_STATE_H
#define RENDER_STRUCTS_DEVICE_STATE_H

#include <volk.h>
#include <mutex>

extern std::mutex graphicsPresentMutex;

struct QueueFamilies {
    uint32_t graphicsPresentFamilyIndex;
    VkQueue graphicsPresentQueue;
};

struct EnabledFeatures {
    bool samplerAnisotropy = false;
    bool sampleRateShading = false;
#ifndef NDEBUG
    bool debugErrorOnly = false;
#endif
};

struct DeviceState {
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    QueueFamilies queue;
    EnabledFeatures features;
};

#endif
