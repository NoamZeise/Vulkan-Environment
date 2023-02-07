#ifndef RENDER_STRUCTS_SWAPCHAIN_FRAME_H
#define RENDER_STRUCTS_SWAPCHAIN_FRAME_H

/// used by Swapchain class as each frame of the swapchain

#include <volk.h>
#include <vector>

#include "attachment_image.h"

class FrameData {
 public:
    FrameData(VkDevice device, uint32_t queueIndex);
    ~FrameData();
    /// first create image views for attachments, getting the memory requirments for the images
    VkResult CreateAttachmentImages(
	    VkImage image, VkFormat swapchainFormat,
	    std::vector<AttachmentImageDescription> &attachDescs,
	    VkExtent2D offscreenExtent,
	    VkDeviceSize *pMemoryRequirements,
	    uint32_t *pMemoryFlagBits,
	    VkSampleCountFlagBits msaaSamples);
    /// call this after creating the memory required by the attachment images
    VkResult CreateAttachmentImageViews(VkDeviceMemory attachmentMemory);
    
    VkResult CreateFramebuffers(VkRenderPass offscreenPass, VkExtent2D offscreenExtent,
				VkRenderPass finalPass, VkExtent2D finalExtent);
    
    void DestroySwapchainResources();

 private:

    void destroyAttachmentImages();
    void destroyAttachments();
				   
    VkDevice device;
    
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore presentReadySem;
    VkFence frameFinishedFence;
    
    //swapchain image - for final render pass
    VkImage swapchainImage;
    VkImageView swapchainImageView;
    VkFramebuffer swapchainFramebuffer;
    
    /// attachments for offscreen render pass
    std::vector<AttachmentImage> attachments;
    
    VkFramebuffer offscreenFramebuffer;
    VkFramebuffer finalFramebuffer;

    enum class FrameDataState {
	Nothing,
	AttachmentImagesCreated,
	AttachmentViewsCreated,
	SwapchainResourcesCreated,
    };
    FrameDataState state = FrameDataState::Nothing;

};

#endif
