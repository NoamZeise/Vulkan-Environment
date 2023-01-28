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
    VkResult InitSwapchainResources(VkPhysicalDevice physicalDevice, VkImage image, VkFormat format);
    void DestroySwapchainResources();

 private:
    VkDevice device;
    bool swapchainInitialized = false;
    
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

};

#endif
