#ifndef VK_ENV_FRAME
#define VK_ENV_FRAME

#include <volk.h>
struct Frame {
    Frame(VkDevice device,  uint32_t graphicsQueueIndex);
    ~Frame();
    
    VkDevice device;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore imageAvailable;
    VkSemaphore presentReady;
    VkFence frameFinished;
};

#endif
