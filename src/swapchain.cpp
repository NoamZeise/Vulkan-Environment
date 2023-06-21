#include "swapchain.h"

#include "parts/swapchain.h"
#include "logger.h"

Swapchain::Swapchain(DeviceState device,
		     VkSurfaceKHR windowSurface,
		     VkExtent2D &windowExtent,
		     RenderConfig conf) {
    this->device = device;
    swapchainImages = part::create::Swapchain(
	    device.device, device.physicalDevice, windowSurface,
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
    vkDestroySwapchainKHR(device.device, swapchain, nullptr);
}

std::vector<VkImage>* Swapchain::getSwapchainImages() {
    return &swapchainImages;
}
