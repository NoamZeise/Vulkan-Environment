#ifndef RENDER_STRUCT_SWAPCHAIN_H
#define RENDER_STRUCT_SWAPCHAIN_H

#include <volk.h>
#include "device_state.h"

#include <vector>

class FrameData;

/// Describes a swapchain with a list of per frame resources
/// allowing for multiple frames to be in flight at any one time
///
/// Each frame has two framebuffers, one (offscreen) that takes drawing commands
/// and renders the scene, and one (final) for drawing to the swapchain image.
class Swapchain {
 public:
    Swapchain(DeviceState deviceState, VkSurfaceKHR windowSurface);
    ~Swapchain();

    /// these are used when the swapchain is out of date
    /// the swapchain will be reused, while all the other resources are remade.
    VkResult InitFrameResources(VkExtent2D windowExtent, VkExtent2D offscreenExtent, bool vsync, bool useSRGB, bool useMultisampling);
    void DestroyFrameResources();

    VkFormat getSwapchainFormat() { return formatKHR.format; }
    VkExtent2D getSwapchainExtent() { return swapchainExtent; }
    VkExtent2D getOffscreenExtent() { return offscreenExtent; }
    VkSampleCountFlagBits getMaxMsaaSamples() { return maxMsaaSamples; }
 
    

 private:
    DeviceState deviceState;
    VkSurfaceKHR windowSurface;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkSurfaceFormatKHR formatKHR;
    VkExtent2D swapchainExtent;
    VkExtent2D offscreenExtent;
    VkSampleCountFlagBits maxMsaaSamples;
    VkDeviceMemory attachmentMemory = VK_NULL_HANDLE;
    std::vector<FrameData> frames;
    bool frameInitialized = false;
};

#endif
