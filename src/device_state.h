#ifndef RENDER_STRUCTS_DEVICE_STATE_H
#define RENDER_STRUCTS_DEVICE_STATE_H

#include <volk.h>

struct QueueFamilies
{
  uint32_t graphicsPresentFamilyIndex;
  VkQueue graphicsPresentQueue;
};

struct EnabledFeatures
{
  bool samplerAnisotropy = false;
  bool sampleRateShading = false;
};

struct DeviceState
{
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  QueueFamilies queue;
  EnabledFeatures features;
};

#endif
