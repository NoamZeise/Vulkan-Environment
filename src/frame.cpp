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
				&commandBuffer, graphicsQueueIndex,
				VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
		"failed to create command pool and buffer for frame");

    checkResultAndThrow(part::create::Semaphore(device, &swapchainImageReady),
		"failed to create image available semaphore");

    checkResultAndThrow(part::create::Semaphore(device, &presentReady),
		"failed to create present ready semaphore");

    checkResultAndThrow(part::create::Fence(device, &frameFinished, true),
		"failed to create frame finished fence");
}

Frame::~Frame() {
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroySemaphore(device, swapchainImageReady, nullptr);
    vkDestroySemaphore(device, presentReady, nullptr);
    vkDestroyFence(device, frameFinished, nullptr);
}

void Frame::waitForPreviousFrame() {
    vkWaitForFences(device, 1, &frameFinished, VK_TRUE, UINT64_MAX);
}

VkResult Frame::startFrame(VkCommandBuffer *pCmdBuff) {
    vkResetFences(device, 1, &frameFinished);
    vkResetCommandBuffer(commandBuffer, 0);
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin.pInheritanceInfo = VK_NULL_HANDLE;
    VkResult result = vkBeginCommandBuffer(commandBuffer, &begin);
    *pCmdBuff = commandBuffer;
    return result;
}
