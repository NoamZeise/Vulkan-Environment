#include "swapchain.h"

#include "../parts/swapchain.h"
#include "../vkhelper.h"
#include "swapchain_frame.h"

#include <iostream>

Swapchain::Swapchain(DeviceState deviceState, VkSurfaceKHR windowSurface) {
    this->deviceState = deviceState;
    this->windowSurface = windowSurface;
}

Swapchain::~Swapchain() {
    if(frameInitialized) { DestroyFrameResources(); }
    frames.clear();
    vkDestroySwapchainKHR(deviceState.device, swapchain, nullptr);
}

VkFormat getDepthBufferFormat(VkPhysicalDevice physicalDevice) {
    return vkhelper::findSupportedFormat(
	    physicalDevice,
	    {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
	    VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkSampleCountFlagBits getMultisampleCount(DeviceState deviceState, bool useMultisampling) {
    if (useMultisampling)
	return vkhelper::getMaxSupportedMsaaSamples(
		deviceState.device, deviceState.physicalDevice);
    else
	return VK_SAMPLE_COUNT_1_BIT;
}

VkResult Swapchain::InitFrameResources(VkExtent2D windowExtent, VkExtent2D offscreenExtent, bool vsync, bool useSRGB, bool useMultisampling) {
    if(swapchain != VK_NULL_HANDLE)
	DestroyFrameResources();

    std::vector<VkImage> images = part::create::Swapchain(
	    deviceState.device,
	    deviceState.physicalDevice,
	    windowSurface, windowExtent.width, windowExtent.height, vsync, useSRGB,
	    &swapchain, &formatKHR, &swapchainExtent);

    VkFormat depthBufferFormat = getDepthBufferFormat(deviceState.physicalDevice);
    if(depthBufferFormat == VK_FORMAT_UNDEFINED) {
	std::cerr << "Error: Depth buffer format was unsupported\n";
	return VK_ERROR_FORMAT_NOT_SUPPORTED;
    }
    
    maxMsaaSamples = getMultisampleCount(deviceState, useMultisampling);
    
    VkDeviceSize attachmentImagesMemorySize;
    uint32_t attachmentImagesMemoryRequirements;
    for(int i = 0; i < images.size(); i++) {
	if(i == frames.size()) 
	    frames.push_back(FrameData(deviceState.device,
				       deviceState.queue.graphicsPresentFamilyIndex));
	else 
	    frames[i].DestroySwapchainResources();
	
	frames[i].CreateAttachmentImages(images[i], formatKHR.format,
					 depthBufferFormat, offscreenExtent,
					 &attachmentImagesMemorySize,
					 &attachmentImagesMemoryRequirements,
					 maxMsaaSamples);
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
