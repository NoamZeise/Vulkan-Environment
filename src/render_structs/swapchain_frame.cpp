#include "swapchain_frame.h"

#include "../parts/command.h"
#include "../parts/threading.h"
#include "../parts/images.h"
#include "../parts/part_macros.h"

#include "../vkhelper.h"

#include <iostream>
#include <stdexcept>
#include <vector>

FrameData::FrameData(VkDevice device, uint32_t queueIndex) {
    this->device = device;
    if(part::create::CommandPoolAndBuffer(device, &commandPool,
					  &commandBuffer, queueIndex) != VK_SUCCESS) {
	throw std::runtime_error("failed to create command pool and buffer for FrameData");
    }
    
    if (part::create::Semaphore(device, &presentReadySem) != VK_SUCCESS)
      throw std::runtime_error("failed to create present ready semaphore");

    if (part::create::Fence(device, &frameFinishedFence, true) != VK_SUCCESS)
      throw std::runtime_error("failed to create frame finished fence");
}

FrameData::~FrameData() {
    if(swapchainInitialized) { DestroySwapchainResources(); }
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroySemaphore(device, presentReadySem, nullptr);
    vkDestroyFence(device, frameFinishedFence, nullptr);
}


VkResult FrameData::createAttachmentResources(VkPhysicalDevice physicalDevice,
					      VkFormat swapchainFormat,
					      VkDeviceSize *pMemoryRequirements,
					      uint32_t *pMemoryFlagBits) {
    VkFormat depthBufferFormat = vkhelper::findSupportedFormat(
	    physicalDevice,
	    {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
	    VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    if(depthBufferFormat == VK_FORMAT_UNDEFINED) {
	std::cerr << "Error: Depth buffer format was unsupported\n";
	return VK_ERROR_FORMAT_NOT_SUPPORTED;
    }

    VkMemoryRequirements memReq;
    
    if(usingMultisamping) {
	multisamplingImage.format = swapchainFormat;
	
    }
    
    //TODO
    return VK_ERROR_INITIALIZATION_FAILED;
}

VkResult FrameData::InitSwapchainResources(
	    VkPhysicalDevice physicalDevice,
	    VkImage image, VkFormat format,
	    VkDeviceSize *pMemoryRequirements,
	    uint32_t *pMemoryFlagBits,
	    bool useMultisampling) {
    VkResult result =  VK_SUCCESS;
    this->usingMultisamping = useMultisampling;
    swapchainImage = image;
    part::create::ImageView(device, &swapchainImageView,
			    swapchainImage, format, VK_IMAGE_ASPECT_COLOR_BIT);

    returnOnErr(createAttachmentResources(physicalDevice,
					  format,
					  pMemoryRequirements,
					  pMemoryFlagBits));
    
    //TODO - init rest of frame resources
    return VK_ERROR_INITIALIZATION_FAILED;

    swapchainInitialized = true;
    return VK_SUCCESS;
}

void FrameData::DestroySwapchainResources() {
    
    vkDestroyImageView(device, swapchainImageView, nullptr);

    swapchainInitialized = false;
}
