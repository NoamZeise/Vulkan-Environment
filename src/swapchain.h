#ifndef VK_ENV_SWAPCHAIN
#define VK_ENV_SWAPCHAIN

#include <vector>
#include <volk.h>
#include "render_structs/device_state.h"
#include <graphics/render_config.h>

class Swapchain {
 public:
    Swapchain(DeviceState device,
	      VkSurfaceKHR windowSurface,
	      //may be modified if the supported extent differs
	      //from the supplied one.
	      VkExtent2D &windowExtent,
	      RenderConfig conf);
    ~Swapchain();
    std::vector<VkImage>* getSwapchainImages();
    
    
 private:
    VkSwapchainKHR swapchain;
    VkExtent2D swapchainExtent;
    VkSurfaceFormatKHR format;
    DeviceState device;
    std::vector<VkImage> images;
};

#endif
