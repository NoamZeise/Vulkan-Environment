#ifndef RENDER_STRUCTS_SWAPCHAIN_FRAME_H
#define RENDER_STRUCTS_SWAPCHAIN_FRAME_H

/// used by Swapchain class as each frame of the swapchain

#include <volk.h>
#include <vector>

struct AttachmentImage
{
  VkImage image;
  VkImageView view;
  VkFormat format;
  size_t memoryOffset;
  
  void destroy(VkDevice device)
  {
    vkDestroyImageView(device, view, nullptr);
    vkDestroyImage(device, image, nullptr);
  }
};


class FrameData {
 public:
    FrameData(VkDevice device, uint32_t queueIndex);
    ~FrameData();
    /// first create image views for attachments, getting the memory requirments for the images
    VkResult CreateAttachmentImages(
	    VkImage image, VkFormat format, VkFormat depthFormat,
	    VkExtent2D offscreenExtent,
	    VkDeviceSize *pMemoryRequirements,
	    uint32_t *pMemoryFlagBits,
	    VkSampleCountFlagBits msaaSamples);
    /// call this after creating the memory required by the attachment images
    VkResult CreateAttachmentImageViews(VkDeviceMemory attachmentMemory);
    VkResult CreateFramebuffers();
    void DestroySwapchainResources();

 private:
    VkResult createAttachmentResources(VkFormat swapchainFormat,
				       VkFormat depthFormat,
				       VkExtent2D offscreenExtent,
				       VkDeviceSize *pMemoryRequirements,
				       uint32_t *pMemoryFlagBits);

    VkResult createAttachmentImage(
	AttachmentImage *pAttachmentImage,
	VkFormat format,
	VkExtent2D extent,
	VkDeviceSize *pMemoryRequirements,
	uint32_t *pMemoryFlagBits,
	VkImageUsageFlags imageUsage,
	VkSampleCountFlagBits msaaSamples);

    VkResult createAttachmentImageView(AttachmentImage* attachmentImage,
					      VkDeviceMemory attachmentMemory,
					      VkImageAspectFlags imageAspectFlags);


    void destroyAttachmentImages();
    void destroyAttachmentViews();
				   
    VkDevice device;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore presentReadySem;
    VkFence frameFinishedFence;
    
    //swapchain image
    VkImage swapchainImage;
    VkImageView swapchainImageView;
    VkFramebuffer swapchainFramebuffer;
    
    /// images for offscreen rendering
    AttachmentImage multisamplingImage;
    AttachmentImage depthBufferImage;
    AttachmentImage offscreenImage;
    VkFramebuffer offscreenFramebuffer;

    enum class FrameDataState {
	Nothing,
	AttachmentImagesCreated,
	AttachmentViewsCreated,
	SwapchainResourcesCreated,
    };
    FrameDataState state = FrameDataState::Nothing;

};

#endif
