#ifndef VK_ENV_SWAPCHAIN
#define VK_ENV_SWAPCHAIN

#include <vector>
#include <volk.h>
#include <graphics/render_config.h>

class Swapchain {
 public:
    Swapchain(VkDevice device, VkPhysicalDevice physicalDevice,
	      VkSurfaceKHR windowSurface,
	      //may be modified if the supported extent differs
	      //from the supplied one.
	      VkExtent2D &windowExtent,
	      RenderConfig conf);
    ~Swapchain();
    std::vector<VkImage>* getSwapchainImages();
    VkResult aquireNextImage(VkSemaphore &imAquired, uint32_t *pImageIndex);
    
    
 private:
    VkSwapchainKHR swapchain;
    VkExtent2D swapchainExtent;
    VkSurfaceFormatKHR format;
    VkDevice device;
    std::vector<VkImage> images;
};

#endif
