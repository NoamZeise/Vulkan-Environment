#include "swapchain_frame.h"

#include "../parts/command.h"
#include "../parts/threading.h"
#include "../parts/images.h"
#include "../parts/part_macros.h"

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
VkResult FrameData::createAttachmentImage(
	AttachmentImage *pAttachmentImage,
	VkFormat format,
	VkExtent2D extent,
	VkDeviceSize *pMemoryRequirements,
	uint32_t *pMemoryFlagBits,
	VkImageUsageFlags imageUsage,
	VkSampleCountFlagBits msaaSamples) {
    VkResult result = VK_SUCCESS;
    VkMemoryRequirements memReq;
    pAttachmentImage->format = format;
    pAttachmentImage->memoryOffset = *pMemoryRequirements;
    returnOnErr(part::create::Image(
			      device,
			      &pAttachmentImage->image,
			      &memReq,
			      imageUsage,
			      extent,
			      pAttachmentImage->format,
			      msaaSamples, 1));
    *pMemoryRequirements += memReq.size;
    *pMemoryFlagBits |= memReq.memoryTypeBits;

    return result;
}
			       

VkResult FrameData::createAttachmentResources(VkFormat swapchainFormat,
					      VkFormat depthFormat,
					      VkExtent2D offscreenExtent,
					      VkDeviceSize *pMemoryRequirements,
					      uint32_t *pMemoryFlagBits) {
    VkResult result = VK_SUCCESS;
    if(msaaSamples != VK_SAMPLE_COUNT_1_BIT) {
	msgAndReturnOnErr(createAttachmentImage(&multisamplingImage, swapchainFormat, offscreenExtent, pMemoryRequirements, pMemoryFlagBits,VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, msaaSamples),
			  "Failed to create multisampling attachment image");
    }

    msgAndReturnOnErr(createAttachmentImage(&depthBufferImage, depthFormat, offscreenExtent, pMemoryRequirements, pMemoryFlagBits, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, msaaSamples),
		      "Failed to create depth buffer attachment image");

    
    msgAndReturnOnErr(createAttachmentImage(&offscreenImage, swapchainFormat, offscreenExtent, pMemoryRequirements, pMemoryFlagBits, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, msaaSamples),
		      "Failed to create offscreen attachment image");
    return result;
}

VkResult FrameData::CreateAttachmentImages(
	VkImage image, VkFormat format, VkFormat depthFormat,
	VkExtent2D offscreenExtent,
	VkDeviceSize *pMemoryRequirements,
	uint32_t *pMemoryFlagBits,
	VkSampleCountFlagBits sampleCount) {
    VkResult result =  VK_SUCCESS;
    this->msaaSamples = sampleCount;
    swapchainImage = image;
    part::create::ImageView(device, &swapchainImageView,
			    swapchainImage, format, VK_IMAGE_ASPECT_COLOR_BIT);
    
    returnOnErr(createAttachmentResources(format, depthFormat,
					  offscreenExtent,
					  pMemoryRequirements,
					  pMemoryFlagBits));
    
    state = FrameDataState::AttachmentImagesCreated;
    return VK_SUCCESS;
}

VkResult FrameData::CreateAttachments() {
    //TODO
    return VK_ERROR_INITIALIZATION_FAILED;
}

void FrameData::destroyAttachmentImages() {
    if(msaaSamples != VK_SAMPLE_COUNT_1_BIT)
	vkDestroyImage(device, multisamplingImage.image, nullptr);
    vkDestroyImage(device, depthBufferImage.image, nullptr);
    vkDestroyImage(device, offscreenImage.image, nullptr);
    vkDestroyImageView(device, swapchainImageView, nullptr);
}

void FrameData::DestroySwapchainResources() {
    
    destroyAttachmentImages();


    state = FrameDataState::Nothing;
}
