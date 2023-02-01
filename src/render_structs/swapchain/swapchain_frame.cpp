#include "swapchain_frame.h"

#include "../../parts/command.h"
#include "../../parts/threading.h"
#include "../../parts/images.h"
#include "../../parts/part_macros.h"

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
    switch(state) {
    case FrameDataState::AttachmentImagesCreated:
	destroyAttachmentImages();
	break;
    case FrameDataState::AttachmentViewsCreated:
	destroyAttachments();
	break;
    case FrameDataState::SwapchainResourcesCreated:
	DestroySwapchainResources();
	break;
    case FrameDataState::Nothing:
	break;
    }
    
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroySemaphore(device, presentReadySem, nullptr);
    vkDestroyFence(device, frameFinishedFence, nullptr);
}			       

VkResult FrameData::CreateAttachmentImages(
	VkImage image, VkFormat swapchainFormat,
	std::vector<AttachmentImageDescription> &attachDescs,
	VkExtent2D offscreenExtent,
	VkDeviceSize *pMemoryRequirements,
	uint32_t *pMemoryFlagBits,
	VkSampleCountFlagBits sampleCount) {
    VkResult result =  VK_SUCCESS;

    swapchainImage = image;
    msgAndReturnOnErr(part::create::ImageView(device, &swapchainImageView,
					       swapchainImage, swapchainFormat,
					      VK_IMAGE_ASPECT_COLOR_BIT),
		       "Failed to create image view for swapchain image");
    
    for(AttachmentImageDescription &attachDesc: attachDescs) {
	AttachmentImage attachmentImage(attachDesc);
	msgAndReturnOnErr(attachmentImage.CreateImage(device, offscreenExtent,
						      pMemoryRequirements, pMemoryFlagBits),
			  "Failed to create attachment image");
    }
    
    state = FrameDataState::AttachmentImagesCreated;
    return VK_SUCCESS;
}

VkResult FrameData::CreateAttachmentImageViews(VkDeviceMemory attachmentMemory) {
    VkResult result = VK_SUCCESS;
    for(auto &a: attachments) {
	msgAndReturnOnErr(a.CreateImageView(device, attachmentMemory),
	    "Failed to create attachment image view");
    }
    state = FrameDataState::AttachmentViewsCreated;
    return result;
}

void FrameData::destroyAttachmentImages() {
    if(state != FrameDataState::AttachmentImagesCreated) {
	throw std::runtime_error("this should only be used if"
				 " attachment images have been created "
				 "but nothing else has been created");
    }
    for(auto &a: attachments)
	vkDestroyImage(device, a.image, nullptr);
    vkDestroyImageView(device, swapchainImageView, nullptr);
}

void FrameData::destroyAttachments() {
    for(auto &a: attachments)
	a.Destroy(device);
    vkDestroyImageView(device, swapchainImageView, nullptr);
}

void FrameData::DestroySwapchainResources() {
    destroyAttachmentImages();

    //TODO framebuffer

    state = FrameDataState::Nothing;
}

