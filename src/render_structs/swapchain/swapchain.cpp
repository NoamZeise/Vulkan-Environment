#include "swapchain.h"

#include "../../parts/swapchain.h"
#include "../../parts/part_macros.h"
#include "../../parts/threading.h"
#include "../../vkhelper.h"
#include "swapchain_frame.h"
#include "render_pass_state.h"

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

std::vector<VkImageView> Swapchain::getOffscreenViews() {
    std::vector<VkImageView> views(frameCount());
    for(size_t i = 0; i < views.size(); i++) {
	views[i] = frames[i].getSwapchainImageView();
    }
    return views;
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

VkResult Swapchain::initFramesAndAttachmentImages(std::vector<VkImage> &images,
						  std::vector<AttachmentImageDescription> &attachDescs) {
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

VkResult createRenderPass(
	VkDevice device,
	std::vector<AttachmentImageDescription> &attachments,
	std::vector<VkSubpassDependency> dependancies,
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


    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = (uint32_t)colourRefs.size();
    subpass.pColorAttachments = colourRefs.data();
    if(resolveRefs.size() > 0)
	subpass.pResolveAttachments = resolveRefs.data();
    if(depthRefs.size() > 0)
	subpass.pDepthStencilAttachment = depthRefs.data();

    VkRenderPassCreateInfo createInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    createInfo.attachmentCount = (uint32_t)attachmentDescriptions.size();
    createInfo.pAttachments = attachmentDescriptions.data();
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = (uint32_t)dependancies.size();
    createInfo.pDependencies = dependancies.data();
    
    return vkCreateRenderPass(device, &createInfo, nullptr, pRenderPass);
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
	getOffscreenAttachmentImageDescriptions(maxMsaaSamples, formatKHR.format, depthBufferFormat);
    
    returnOnErr(initFramesAndAttachmentImages(images, attachmentsDesc));

    for(auto& frame: frames) {
	frame.CreateAttachmentImageViews(attachmentMemory);
    }


    msgAndReturnOnErr(createRenderPass(deviceState.device, attachmentsDesc,
				       getOffscreenSubpassDependancies(),
				       &offscreenRenderPass),
		      "Failed to create offscreen Render Pass");

    auto finalAttachments = getFinalAttachmentImageDescriptions(formatKHR.format);
    msgAndReturnOnErr(createRenderPass(deviceState.device,
				       finalAttachments,
				       getFinalSubpassDependancies(),
				       &finalRenderPass),
    		      "Failed to create final Render Pass");


    for(FrameData &f: frames) {
	f.CreateFramebuffers(offscreenRenderPass, offscreenExtent, finalRenderPass, swapchainExtent);
    }
    
    return VK_ERROR_INITIALIZATION_FAILED;
    
    this->offscreenExtent = offscreenExtent;
    frameInitialized = true;
    return result;
}

void Swapchain::DestroyFrameResources() {
    if(swapchain != VK_NULL_HANDLE) {
	// not sure that 
    }

    for(auto& frame: frames) {
	frame.DestroySwapchainResources();
    }

    frameInitialized = false;
}



VkResult Swapchain::beginOffscreenRenderPass(VkCommandBuffer *pCmdBuff, VkSemaphore *pImageAquireSem) {
    VkResult result = VK_SUCCESS;
    if (imageAquireSemaphores.empty()) {
	part::create::Semaphore(deviceState.device, pImageAquireSem);
    } else {
	*pImageAquireSem = imageAquireSemaphores.back();
	imageAquireSemaphores.pop_back();
    }

    result = vkAcquireNextImageKHR(deviceState.device, swapchain, UINT64_MAX, *pImageAquireSem,
				   nullptr, &frameIndex);
    if(result != VK_SUCCESS) {
	imageAquireSemaphores.push_back(*pImageAquireSem);
	return result;
    }

    returnOnErr(frames[frameIndex].startFrame(pCmdBuff));

    VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassInfo.renderPass = offscreenRenderPass;
    renderPassInfo.framebuffer = frames[frameIndex].getOffscreenFramebuffer();
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = offscreenExtent;
    VkClearValue clearColours[2];
    clearColours[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearColours[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearColours;

    vkCmdBeginRenderPass(*pCmdBuff, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)offscreenExtent.width;
    viewport.height = (float)offscreenExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(*pCmdBuff, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = offscreenExtent;
    vkCmdSetScissor(*pCmdBuff, 0, 1, &scissor);
    
    return result;
}

//TODO during frame and end frame swapchain