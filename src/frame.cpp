#include "frame.h"
#include "logger.h"
#include "parts/command.h"
#include "parts/threading.h"
#include <stdexcept>

//TODO: Single command pool for all frames

Frame::Frame(VkDevice device,  uint32_t graphicsQueueIndex) {
    this->device = device;
    checkResultAndThrow(part::create::CommandPoolAndBuffer(
				device, &commandPool,
				&commandBuffer, graphicsQueueIndex, 0),
		"failed to create command pool and buffer for frame");

    checkResultAndThrow(part::create::Semaphore(device, &swapchainImageReady),
		"failed to create image available semaphore");

    checkResultAndThrow(part::create::Semaphore(device, &drawFinished),
		"failed to create present ready semaphore");

    checkResultAndThrow(part::create::Fence(device, &frameFinished, true),
		"failed to create frame finished fence");
}

Frame::~Frame() {
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroySemaphore(device, swapchainImageReady, nullptr);
    vkDestroySemaphore(device, drawFinished, nullptr);
    vkDestroyFence(device, frameFinished, nullptr);
}

VkResult Frame::waitForPreviousFrame() {
    VkResult result = vkWaitForFences(device, 1, &frameFinished, VK_TRUE, UINT64_MAX);
    if(result != VK_SUCCESS)
	LOG_ERR_TYPE("Failed to wait for frame fence", result);
    result = vkResetFences(device, 1, &frameFinished);
    if(result != VK_SUCCESS)
	LOG_ERR_TYPE("Failed to reset frame fence", result);
    return result;
}

VkResult Frame::startFrame(VkCommandBuffer *pCmdBuff) {
    vkResetCommandPool(device, commandPool, 0);
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin.pInheritanceInfo = VK_NULL_HANDLE;
    VkResult result = vkBeginCommandBuffer(commandBuffer, &begin);
    *pCmdBuff = commandBuffer;
    return result;
}
