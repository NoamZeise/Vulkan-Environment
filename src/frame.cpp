#include "frame.h"
#include "logger.h"
#include "parts/command.h"
#include "parts/threading.h"
#include <stdexcept>

void checkResult(VkResult result, std::string message);

Frame::Frame(VkDevice device,  uint32_t graphicsQueueIndex) {
    this->device = device;
    checkResult(part::create::CommandPoolAndBuffer(device, &commandPool,
						   &commandBuffer, graphicsQueueIndex),
		"failed to create command pool and buffer for frame");

    checkResult(part::create::Semaphore(device, &imageAvailable),
		"failed to create image available semaphore");

    checkResult(part::create::Semaphore(device, &presentReady),
		"failed to create present ready semaphore");

    checkResult(part::create::Fence(device, &frameFinished, true),
		"failed to create frame finished fence");
}

Frame::~Frame() {
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroySemaphore(device, imageAvailable, nullptr);
    vkDestroySemaphore(device, presentReady, nullptr);
    vkDestroyFence(device, frameFinished, nullptr);
}

void checkResult(VkResult result, std::string message) {
    if(result != VK_SUCCESS) {
	LOG_ERR_TYPE(message, result);
	throw std::runtime_error(message);
    }
}
