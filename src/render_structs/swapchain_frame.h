#ifndef RENDER_STRUCTS_SWAPCHAIN_FRAME_H
#define RENDER_STRUCTS_SWAPCHAIN_FRAME_H

/// used by Swapchain class as each frame of the swapchain

#include <volk.h>

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
    VkResult CreateAttachmentImages(
	    VkImage image, VkFormat format, VkFormat depthFormat,
	    VkExtent2D offscreenExtent,
	    VkDeviceSize *pMemoryRequirements,
	    uint32_t *pMemoryFlagBits,
	    VkSampleCountFlagBits msaaSamples);
    VkResult CreateAttachments();
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

    void destroyAttachmentImages();
				   
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
	SwapchainResourcesCreated,
    };
    FrameDataState state = FrameDataState::Nothing;

};

#endif
