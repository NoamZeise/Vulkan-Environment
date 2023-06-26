#include "frame.h"
#include "logger.h"
#include "parts/command.h"
#include "parts/threading.h"
#include <stdexcept>

Frame::Frame(VkDevice device,  uint32_t graphicsQueueIndex) {
    this->device = device;
    checkResultAndThrow(part::create::CommandPoolAndBuffer(device, &commandPool,
						   &commandBuffer, graphicsQueueIndex),
		"failed to create command pool and buffer for frame");

    checkResultAndThrow(part::create::Semaphore(device, &imageAvailable),
		"failed to create image available semaphore");

    checkResultAndThrow(part::create::Semaphore(device, &presentReady),
		"failed to create present ready semaphore");

    checkResultAndThrow(part::create::Fence(device, &frameFinished, true),
		"failed to create frame finished fence");
}

Frame::~Frame() {
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroySemaphore(device, imageAvailable, nullptr);
    vkDestroySemaphore(device, presentReady, nullptr);
    vkDestroyFence(device, frameFinished, nullptr);
}

VkResult Frame::startFrame(VkCommandBuffer *pCmdBuff) {
    vkWaitForFences(device, 1, &frameFinished, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &frameFinished);

    vkResetCommandPool(device, commandPool, 0);
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin.pInheritanceInfo = VK_NULL_HANDLE;
    VkResult result = vkBeginCommandBuffer(commandBuffer, &begin);
    *pCmdBuff = commandBuffer;
    return result;
}
