#include "swapchain.h"

#include "parts/swapchain.h"
#include "parts/images.h"
#include "logger.h"

Swapchain::Swapchain(DeviceState device,
		     VkSurfaceKHR windowSurface,
		     VkExtent2D &windowExtent,
		     RenderConfig conf) {
    this->device = device;
    std::vector<VkImage> swapchainImages = part::create::Swapchain(
	    device.device, device.physicalDevice, windowSurface,
	    windowExtent.width, windowExtent.height, conf.vsync, conf.srgb,
	    &swapchain, &this->format, &this->swapchainExtent);
    if(swapchainExtent.width != windowExtent.width ||
       swapchainExtent.height != windowExtent.height) {
	LOG("Supported Swapchain Extent differs from window extent!");
	windowExtent.width = swapchainExtent.width;
	windowExtent.height = swapchainExtent.height;
    }

    for(VkImage& img: swapchainImages) {
	VkImageView view;
	VkResult result = part::create::ImageView(device.device, &view, img,
						  format.format,
						  VK_IMAGE_ASPECT_COLOR_BIT);
	checkResultAndThrow(result, "Error creating Image View for swapchain Image");
        this->images.push_back(view);
    }
}


Swapchain::~Swapchain() {
    vkDestroySwapchainKHR(device.device, swapchain, VK_NULL_HANDLE);
    for(VkImageView &view: images) {
	vkDestroyImageView(device.device, view, VK_NULL_HANDLE);
    }
}

std::vector<VkImageView>* Swapchain::getSwapchainImageViews() {
    return &images;
}
