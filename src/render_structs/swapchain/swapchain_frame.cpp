#include "swapchain_frame.h"

#include "../../parts/command.h"
#include "../../parts/threading.h"
#include "../../parts/images.h"
#include "../../logger.h"
#include "../../parts/framebuffer.h"


#include <iostream>
#include <stdexcept>
#include <vector>

FrameData::FrameData(VkDevice device, uint32_t queueIndex) {
    this->device = device;
    if(part::create::CommandPoolAndBuffer(device, &commandPool,
					  &commandBuffer, queueIndex) != VK_SUCCESS)
	throw std::runtime_error("failed to create command pool and buffer for FrameData");

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


VkImageView FrameData::getOffscreenImageView() {
    if(state != FrameDataState::SwapchainResourcesCreated || attachments.size() < 1)
	throw std::runtime_error("tried to get swapchain image view from frame"
				 " that hasn't finished being created!");
    // first attachment is either colour or resolve,
    // it will always be the one being sampled
    // by the final render pass
    return attachments[0].view;
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
	attachments.push_back(AttachmentImage(attachDesc));
	msgAndReturnOnErr(attachments.back().CreateImage(
				  device, offscreenExtent, pMemoryRequirements, pMemoryFlagBits),
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

VkResult FrameData::CreateFramebuffers(VkRenderPass offscreenPass, VkExtent2D offscreenExtent, VkRenderPass finalPass, VkExtent2D finalExtent) {
    VkResult result = VK_SUCCESS;

    std::vector<VkImageView> offscreenViews;
    for(auto &a: attachments)
	offscreenViews.push_back(a.view);

    msgAndReturnOnErr(part::create::Framebuffer(
			      device, offscreenPass, &offscreenFramebuffer, offscreenViews,
			      offscreenExtent.width, offscreenExtent.height),
		      "Failed to create offscreen framebuffer for FrameData");
    
    msgAndReturnOnErr(part::create::Framebuffer(
			      device, finalPass, &finalFramebuffer, {swapchainImageView},
			      finalExtent.width, finalExtent.height),
		      "Failed to create final framebuffer for FrameData");
    state = FrameDataState::SwapchainResourcesCreated;
    return result;
}


void FrameData::destroyAttachmentImages() {
    if(state != FrameDataState::AttachmentImagesCreated) {
	throw std::runtime_error("this should only be used if"
				 " attachment images have been created "
				 "but nothing else has been created,"
				 "use destroyAttachments() instead");
    }
    for(auto &a: attachments)
	vkDestroyImage(device, a.image, nullptr);
    attachments.clear();
    vkDestroyImageView(device, swapchainImageView, nullptr);
}

void FrameData::destroyAttachments() {
    for(auto &a: attachments)
	a.Destroy(device);
    attachments.clear();
    vkDestroyImageView(device, swapchainImageView, nullptr);
}

void FrameData::DestroySwapchainResources() {
    vkDestroyFramebuffer(device, offscreenFramebuffer, nullptr);
    vkDestroyFramebuffer(device, finalFramebuffer, nullptr);
    destroyAttachments();
    state = FrameDataState::Nothing;
}

VkResult FrameData::startFrame(VkCommandBuffer *pCmdBuff) {
    VkResult result = VK_SUCCESS;
    
    vkWaitForFences(device, 1, &frameFinishedFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &frameFinishedFence);

    vkResetCommandPool(device, commandPool, 0);
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;
    msgAndReturnOnErr(vkBeginCommandBuffer(commandBuffer, &beginInfo),
		      "Failed to begin command buffer for frame");

    *pCmdBuff = commandBuffer;
    return result;
}

