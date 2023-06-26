#include "swapchain.h"

#include "parts/swapchain.h"
#include "parts/images.h"
#include "logger.h"

Swapchain::Swapchain(VkDevice device, VkPhysicalDevice physicalDevice,
		     VkSurfaceKHR windowSurface,
		     VkExtent2D &windowExtent,
		     RenderConfig conf) {
    this->device = device;
    images = part::create::Swapchain(
	    device, physicalDevice, windowSurface,
	    windowExtent.width, windowExtent.height, conf.vsync, conf.srgb,
	    &swapchain, &this->format, &this->swapchainExtent);
    if(swapchainExtent.width != windowExtent.width ||
       swapchainExtent.height != windowExtent.height) {
	LOG("Supported Swapchain Extent differs from window extent!");
	windowExtent.width = swapchainExtent.width;
	windowExtent.height = swapchainExtent.height;
    }
}


Swapchain::~Swapchain() {
    vkDestroySwapchainKHR(device, swapchain, VK_NULL_HANDLE);
}

std::vector<VkImage>* Swapchain::getSwapchainImages() {
    return &images;
}

VkResult Swapchain::aquireNextImage(VkSemaphore &imAquired, uint32_t *pImageIndex) {
    return vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imAquired, VK_NULL_HANDLE,
				 pImageIndex);
}
