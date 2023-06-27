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
    void RecreateSwapchain(VkExtent2D &windowExtent, RenderConfig &conf);
    std::vector<VkImage>* getSwapchainImages();
    VkSwapchainKHR getSwapchain();
    VkFormat getFormat();
    VkResult acquireNextImage(VkSemaphore imAquired, uint32_t *pImageIndex);
    
    
 private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkExtent2D swapchainExtent;
    VkSurfaceFormatKHR format;
    std::vector<VkImage> images;
};

#endif
