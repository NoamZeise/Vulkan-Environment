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
