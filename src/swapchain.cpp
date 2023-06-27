#include "swapchain.h"

#include "parts/swapchain.h"
#include "parts/images.h"
#include "logger.h"

Swapchain::Swapchain(VkDevice device, VkPhysicalDevice physicalDevice,
		     VkSurfaceKHR windowSurface,
		     VkExtent2D &windowExtent,
		     RenderConfig conf) {
    this->device = device;
    this->physicalDevice = physicalDevice;
    this->surface = windowSurface;
    RecreateSwapchain(windowExtent, conf);
}

Swapchain::~Swapchain() {
    vkDestroySwapchainKHR(device, swapchain, VK_NULL_HANDLE);
}

void Swapchain::RecreateSwapchain(VkExtent2D &windowExtent, RenderConfig &conf) {
    images = part::create::Swapchain(
	    device, physicalDevice, surface,
	    windowExtent.width, windowExtent.height, conf.vsync, conf.srgb,
	    &swapchain, &this->format, &this->swapchainExtent);
    if(swapchainExtent.width != windowExtent.width ||
       swapchainExtent.height != windowExtent.height) {
	LOG("Supported Swapchain Extent differs from window extent!");
	windowExtent.width = swapchainExtent.width;
	windowExtent.height = swapchainExtent.height;
    }
}

std::vector<VkImage> *Swapchain::getSwapchainImages() { return &images; }

VkSwapchainKHR Swapchain::getSwapchain() {
    return this->swapchain;
}

VkFormat Swapchain::getFormat() {
    return this->format.format;
}

VkResult Swapchain::acquireNextImage(VkSemaphore imAquired, uint32_t *pImageIndex) {
    return vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imAquired, VK_NULL_HANDLE,
				 pImageIndex);
}
