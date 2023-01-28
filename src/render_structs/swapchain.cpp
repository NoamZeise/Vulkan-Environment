#include "swapchain.h"

#include "../parts/swapchain.h"

#include "swapchain_frame.h"

Swapchain::Swapchain(DeviceState deviceState, VkSurfaceKHR windowSurface) {
    this->deviceState = deviceState;
    this->windowSurface = windowSurface;
}

Swapchain::~Swapchain() {
    if(frameInitialized) { DestroyFrameResources(); }
    frames.clear();
    vkDestroySwapchainKHR(deviceState.device, swapchain, nullptr);
}

VkResult Swapchain::InitFrameResources(VkExtent2D windowExtent, VkExtent2D offscreenExtent, bool vsync, bool useSRGB, bool useMultisampling) {
    if(swapchain != VK_NULL_HANDLE)
	DestroyFrameResources();

    std::vector<VkImage> images = part::create::Swapchain(
	    deviceState.device,
	    deviceState.physicalDevice,
	    windowSurface, windowExtent.width, windowExtent.height, vsync, useSRGB,
	    &swapchain, &formatKHR, &swapchainExtent);


    VkDeviceSize attachmentImagesMemorySize;
    uint32_t attachmentImagesMemoryRequirements;
    for(int i = 0; i < images.size(); i++) {
	if(i == frames.size()) 
	    frames.push_back(FrameData(deviceState.device,
				       deviceState.queue.graphicsPresentFamilyIndex));
	else 
	    frames[i].DestroySwapchainResources();
	
	frames[i].InitSwapchainResources(deviceState.physicalDevice,
					 images[i], formatKHR.format,
					 &attachmentImagesMemorySize,
					 &attachmentImagesMemoryRequirements,
					 useMultisampling);
    }
    while(images.size() < frames.size())
	frames.pop_back();
    
    //TODO
    return VK_ERROR_INITIALIZATION_FAILED;
    
    this->offscreenExtent = offscreenExtent;
    frameInitialized = true;
    return VK_SUCCESS;
}

void Swapchain::DestroyFrameResources() {
    if(swapchain != VK_NULL_HANDLE) {

    }

    frameInitialized = false;
}
