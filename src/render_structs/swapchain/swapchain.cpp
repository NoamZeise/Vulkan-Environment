#include "swapchain.h"

#include "../../parts/swapchain.h"
#include "../../parts/part_macros.h"
#include "../../vkhelper.h"
#include "swapchain_frame.h"

#include <iostream>

Swapchain::Swapchain(DeviceState deviceState, VkSurfaceKHR windowSurface) {
    this->deviceState = deviceState;
    this->windowSurface = windowSurface;
}

Swapchain::~Swapchain() {
    if(frameInitialized) { DestroyFrameResources(); }
    frames.clear();
    vkDestroySwapchainKHR(deviceState.device, swapchain, nullptr);
}

VkFormat getDepthBufferFormat(VkPhysicalDevice physicalDevice) {
    return vkhelper::findSupportedFormat(
	    physicalDevice,
	    {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
	    VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkSampleCountFlagBits getMultisampleCount(DeviceState deviceState, bool useMultisampling) {
    if (useMultisampling)
	return vkhelper::getMaxSupportedMsaaSamples(
		deviceState.device, deviceState.physicalDevice);
    else
	return VK_SAMPLE_COUNT_1_BIT;
}

VkResult Swapchain::initFramesAndAttachmentImages(std::vector<VkImage> &images, std::vector<AttachmentImageDescription> &attachDescs) {
    VkResult result = VK_SUCCESS;
    VkDeviceSize attachmentImagesMemorySize;
    uint32_t attachmentImagesMemoryRequirements;
    for(int i = 0; i < images.size(); i++) {
	if(i == frames.size()) 
	    frames.push_back(FrameData(deviceState.device,
				       deviceState.queue.graphicsPresentFamilyIndex));
	else 
	    frames[i].DestroySwapchainResources();
	
	returnOnErr(frames[i].CreateAttachmentImages(images[i], formatKHR.format,
					 attachDescs, offscreenExtent,
					 &attachmentImagesMemorySize,
					 &attachmentImagesMemoryRequirements,
							   maxMsaaSamples));
    }
    
    while(images.size() < frames.size())
	frames.pop_back();
    
    msgAndReturnOnErr(vkhelper::createMemory(deviceState.device, deviceState.physicalDevice,
					     attachmentImagesMemorySize,
					     &this->attachmentMemory,
					     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				    attachmentImagesMemoryRequirements),
	"Failed to create memory for swapchain frame attachment images"); 
    
    return result;
}

std::vector<AttachmentImageDescription> getAttachmentImageDescriptions(VkSampleCountFlagBits samples, VkFormat swapchainFormat, VkFormat depthFormat) {
    std::vector<AttachmentImageDescription> attachments;
    uint32_t attachmentIndex = 0;
    if(samples != VK_SAMPLE_COUNT_1_BIT) {
	AttachmentImageDescription multisampling(attachmentIndex++, AttachmentImageType::Colour);
	multisampling.imageUsageFlags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	multisampling.samples = samples;
	multisampling.format = swapchainFormat;
	multisampling.finalImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	multisampling.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	multisampling.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments.push_back(multisampling);
    }
    AttachmentImageDescription depth(attachmentIndex++, AttachmentImageType::Depth);
    depth.samples = samples;
    depth.format = depthFormat;
    depth.finalImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments.push_back(depth);

    AttachmentImageType type = samples == VK_SAMPLE_COUNT_1_BIT ?
	AttachmentImageType::Colour : AttachmentImageType::Resolve;
    AttachmentImageDescription resolve(attachmentIndex++, type);
    resolve.samples = samples;
    resolve.format = swapchainFormat;
    resolve.finalImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //TODO check if this is nessecary
    if(resolve.type == AttachmentImageType::Resolve)
	resolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    else
	resolve.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    resolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments.push_back(resolve);
    
    return attachments;
}

VkResult createRenderPass(
	VkDevice device,
	std::vector<AttachmentImageDescription> &attachments,
	VkRenderPass *pRenderPass) {
    std::vector<VkAttachmentReference> colourRefs;
    std::vector<VkAttachmentReference> depthRefs;
    std::vector<VkAttachmentReference> resolveRefs;
    std::vector<VkAttachmentDescription> attachmentDescriptions;
    for(auto &a: attachments) {
	switch(a.type) {
	case AttachmentImageType::Colour:
	    colourRefs.push_back(a.getAttachmentReference());
	    break;
	case AttachmentImageType::Depth:
	    depthRefs.push_back(a.getAttachmentReference());
	case AttachmentImageType::Resolve:
	    resolveRefs.push_back(a.getAttachmentReference());
	}
	attachmentDescriptions.push_back(a.getAttachmentDescription());
    }

    //TODO create subpass, dependencies, renderpass
    return VK_ERROR_INITIALIZATION_FAILED;
}

VkResult Swapchain::InitFrameResources(VkExtent2D windowExtent, VkExtent2D offscreenExtent, bool vsync, bool useSRGB, bool useMultisampling) {
    VkResult result = VK_SUCCESS;
    if(swapchain != VK_NULL_HANDLE)
	DestroyFrameResources();

    std::vector<VkImage> images = part::create::Swapchain(
	    deviceState.device,
	    deviceState.physicalDevice,
	    windowSurface, windowExtent.width, windowExtent.height, vsync, useSRGB,
	    &swapchain, &formatKHR, &swapchainExtent);

    VkFormat depthBufferFormat = getDepthBufferFormat(deviceState.physicalDevice);
    if(depthBufferFormat == VK_FORMAT_UNDEFINED) {
	std::cerr << "Error: Depth buffer format was unsupported\n";
	return VK_ERROR_FORMAT_NOT_SUPPORTED;
    }

    maxMsaaSamples = getMultisampleCount(deviceState, useMultisampling);

    std::vector<AttachmentImageDescription> attachmentsDesc =
	getAttachmentImageDescriptions(maxMsaaSamples, formatKHR.format, depthBufferFormat);
    
    returnOnErr(initFramesAndAttachmentImages(images, attachmentsDesc));

    for(auto& frame: frames) {
	frame.CreateAttachmentImageViews(attachmentMemory);
    }


    msgAndReturnOnErr(createRenderPass(deviceState.device, attachmentsDesc, &offscreenRenderPass),
		      "Failed to create offscreen Render Pass");
    
    //TODO
    return VK_ERROR_INITIALIZATION_FAILED;
    
    this->offscreenExtent = offscreenExtent;
    frameInitialized = true;
    return result;
}

void Swapchain::DestroyFrameResources() {
    if(swapchain != VK_NULL_HANDLE) {
	
    }

    for(auto& frame: frames) {
	frame.DestroySwapchainResources();
    }

    frameInitialized = false;
}
