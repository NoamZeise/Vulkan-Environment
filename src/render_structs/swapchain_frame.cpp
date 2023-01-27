#include "swapchain_frame.h"

#include "../parts/command.h"
#include "../parts/threading.h"
#include "../parts/images.h"

#include <stdexcept>

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

VkResult FrameData::InitSwapchainResources(VkImage image, VkFormat format) {
    swapchainImage = image;
    part::create::ImageView(device, &swapchainImageView,
			    swapchainImage, format, VK_IMAGE_ASPECT_COLOR_BIT);

    //TODO - init rest of frame resources
    return VK_ERROR_INITIALIZATION_FAILED;

    swapchainInitialized = true;
    return VK_SUCCESS;
}

void FrameData::DestroySwapchainResources() {
    
    vkDestroyImageView(device, swapchainImageView, nullptr);

    swapchainInitialized = false;
}
