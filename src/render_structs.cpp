#include "render_structs.h"

#include <config.h>
#include "parts/swapchain.h"

VkResult SwapChain::initResources(DeviceState &deviceState, VkSurfaceKHR windowSurface, uint32_t windowWidth, uint32_t windowHeight, bool vsync, VkExtent2D offscreenExtent) {
    if(swapChain != VK_NULL_HANDLE)
	destroyResources(deviceState.device);

    std::vector<VkImage> images = part::create::Swapchain(
	    deviceState.device,
	    deviceState.physicalDevice,
	    windowSurface, windowWidth, windowHeight, vsync, settings::SRGB,
		&swapChain, &format, &swapchainExtent);
    
    frameData.resize(images.size());
    
    for(int i = 0; i < frameData.size(); i++) {
	frameData[i].SetPerFramData(deviceState.device, images[i], format.format,
				    deviceState.queue.graphicsPresentFamilyIndex);
    }
    
    this->offscreenExtent = offscreenExtent;
    
    return VK_SUCCESS;
}
