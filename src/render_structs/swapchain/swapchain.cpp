#include "swapchain.h"

#include "../../parts/swapchain.h"
#include "../../parts/part_macros.h"
#include "../../parts/threading.h"
#include "../../vkhelper.h"
#include "swapchain_frame.h"
#include "render_pass_state.h"

#include <iostream>
#include <fstream>

Swapchain::Swapchain(DeviceState deviceState, VkSurfaceKHR windowSurface) {
    this->deviceState = deviceState;
    this->windowSurface = windowSurface;
}

Swapchain::~Swapchain() {
    if(currentState != FrameState::NothingInitialized) { DestroyFrameResources(); }
    for(int i = 0; i < frames.size(); i++) {
	delete frames[i];
    }
    for(int i = 0; i < imageAquireSemaphores.size(); i++) {
	vkDestroySemaphore(deviceState.device, imageAquireSemaphores[i], nullptr);
    }
    
    vkDestroySwapchainKHR(deviceState.device, swapchain, nullptr);
}

std::vector<VkImageView> Swapchain::getOffscreenViews() {
    std::vector<VkImageView> views(frameCount());
    for(size_t i = 0; i < views.size(); i++) {
	views[i] = frames[i]->getOffscreenImageView();
    }
    return views;
}

size_t Swapchain::frameCount() { return frames.size(); }

uint32_t Swapchain::getFrameIndex() {  return frameIndex; }

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
    VkDeviceSize attachmentImagesMemorySize  = 0;
    uint32_t attachmentImagesMemoryRequirements = 0;
    for(int i = 0; i < images.size(); i++) {
	if(i == frames.size()) {
	    frames.push_back(new FrameData(deviceState.device,
				       deviceState.queue.graphicsPresentFamilyIndex));
	}
	
	returnOnErr(frames[i]->CreateAttachmentImages(images[i], formatKHR.format,
					 attachDescs, offscreenExtent,
					 &attachmentImagesMemorySize,
					 &attachmentImagesMemoryRequirements,
							   maxMsaaSamples));
    }
    
    while(images.size() < frames.size()) {
	delete frames.back();
	frames.pop_back();
    }
    
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
	    break;
	case AttachmentImageType::Resolve:
	    resolveRefs.push_back(a.getAttachmentReference());
	    break;
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

VkResult Swapchain::InitFrameResources(VkExtent2D windowExtent, VkExtent2D offscreenExtent,
				       bool vsync, bool useSRGB, bool useMultisampling) {
    if(currentState != FrameState::NothingInitialized) {
	std::cerr << "Error: "
	    "tried to create frame resources when they already exist\n";
	return  VK_ERROR_UNKNOWN;
    }
    this->offscreenExtent = offscreenExtent;
    VkResult result = VK_SUCCESS;

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

    attachmentClearValues.clear();
    for(AttachmentImageDescription &a: attachmentsDesc)  {
	//not all attachments will use these clear values, but easier to just give them all one
	VkClearValue clear;
	switch(a.type) {
	case AttachmentImageType::Colour:
	case AttachmentImageType::Resolve:
	    clear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
	    break;
	case AttachmentImageType::Depth:
	    clear.depthStencil = {1.0f, 0};
	    break;
	}
	attachmentClearValues.push_back(clear);
    }
    
    returnOnErr(initFramesAndAttachmentImages(images, attachmentsDesc));
    
    for(FrameData *f: frames) {
	f->CreateAttachmentImageViews(attachmentMemory);
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

    
    for(FrameData *f: frames) {
	f->CreateFramebuffers(offscreenRenderPass, offscreenExtent,
			      finalRenderPass, swapchainExtent);
    }

    currentState = FrameState::FrameResourcesCreated;
    return result;
}

void Swapchain::DestroyFrameResources() {
    if(currentState != FrameState::FrameResourcesCreated) {
	throw std::runtime_error("Tried to destroy frame resources"
				 " while render pass is running "
				 "or frame resources are uncreated!");
    }
    //TODO: optimize swapchain recreation for reuse 

    vkDestroyRenderPass(deviceState.device, offscreenRenderPass,  nullptr);
    vkDestroyRenderPass(deviceState.device, finalRenderPass,  nullptr);
    
    for(FrameData *f: frames) {
	f->DestroySwapchainResources();
    }
    vkFreeMemory(deviceState.device,  attachmentMemory,  nullptr);
    
    currentState = FrameState::NothingInitialized;
}


VkViewport getViewport(VkExtent2D extent) {
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    return viewport;
}

VkRect2D getScissor(VkExtent2D extent) {
    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = extent;
    return scissor;
}

VkResult Swapchain::beginOffscreenRenderPass(VkCommandBuffer *pCmdBuff) {
    if(currentState != FrameState::FrameResourcesCreated) {
	std::cerr << "Error: "
	    "tried to begin offscreen render pass without creating frame resources\n";
	return  VK_ERROR_UNKNOWN;
    }
      
    VkResult result = VK_SUCCESS;
    if (imageAquireSemaphores.empty()) {
	part::create::Semaphore(deviceState.device, &currentImgAquireSem);
    } else {
	currentImgAquireSem = imageAquireSemaphores.back();
	imageAquireSemaphores.pop_back();
    }

    result = vkAcquireNextImageKHR(deviceState.device, swapchain, UINT64_MAX, currentImgAquireSem,
				   nullptr, &frameIndex);
    if(result != VK_SUCCESS) {
	imageAquireSemaphores.push_back(currentImgAquireSem);
	return result;
    }

    returnOnErr(frames[frameIndex]->startFrame(pCmdBuff));

    VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassInfo.renderPass = offscreenRenderPass;
    renderPassInfo.framebuffer = frames[frameIndex]->getOffscreenFramebuffer();
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = offscreenExtent;

    renderPassInfo.clearValueCount = (uint32_t)attachmentClearValues.size();
    renderPassInfo.pClearValues = attachmentClearValues.data();

    vkCmdBeginRenderPass(*pCmdBuff, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    VkViewport viewport = getViewport(offscreenExtent);
    vkCmdSetViewport(*pCmdBuff, 0, 1, &viewport);
    VkRect2D scissor = getScissor(offscreenExtent);
    vkCmdSetScissor(*pCmdBuff, 0, 1, &scissor);

    currentState = FrameState::OffscreenPassBegan;
    return result;
}

void Swapchain::endOffscreenRenderPassAndBeginFinal() {
    if(currentState != FrameState::OffscreenPassBegan) {
	throw std::runtime_error("Tried to end offscreen render pass before it began!");
    }
    VkCommandBuffer cmdbuff = frames[frameIndex]->getCmdBuff();
    vkCmdEndRenderPass(cmdbuff);

    VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassInfo.renderPass = finalRenderPass;
    renderPassInfo.framebuffer = frames[frameIndex]->getSwapchainFramebuffer();
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchainExtent;
    VkClearValue clear = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clear;

    vkCmdBeginRenderPass(cmdbuff,
			 &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = getViewport(swapchainExtent);
    vkCmdSetViewport(cmdbuff, 0, 1, &viewport);
    VkRect2D scissor = getScissor(swapchainExtent);
    vkCmdSetScissor(cmdbuff, 0, 1, &scissor);

    currentState = FrameState::FinalPassBegan;
}


VkResult Swapchain::endFinalRenderPass() {
    if(currentState != FrameState::FinalPassBegan) {
	std::cerr << "Error:  tried to end final render pass before it began\n";
	return  VK_ERROR_UNKNOWN;
    }
    VkResult result = VK_SUCCESS;
    VkCommandBuffer cmdbuff = frames[frameIndex]->getCmdBuff();
    vkCmdEndRenderPass(cmdbuff);

    msgAndReturnOnErr(vkEndCommandBuffer(cmdbuff), "Failed to end frame command buffer");

    //Submit draw command
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &currentImgAquireSem;
    VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &stageFlags;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdbuff;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &frames[frameIndex]->presentReadySem;
    msgAndReturnOnErr(
	    vkQueueSubmit(deviceState.queue.graphicsPresentQueue, 1, &submitInfo,
			  frames[frameIndex]->frameFinishedFence),
	    "Failed to submit graphics queue");

    //Submit present command
    VkPresentInfoKHR presentInfo {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &frames[frameIndex]->presentReadySem;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &frameIndex;
    result = vkQueuePresentKHR(deviceState.queue.graphicsPresentQueue, &presentInfo);

    imageAquireSemaphores.push_back(currentImgAquireSem);
    currentState =  FrameState::FrameResourcesCreated;
    return result;
}
