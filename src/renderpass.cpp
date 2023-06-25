#include "renderpass.h"

#include <stdexcept>

AttachmentDesc::AttachmentDesc(uint32_t index, AttachmentType type, AttachmentUse use,
			       VkSampleCountFlagBits sampleCount, VkFormat format) {
    this->index = index;
    this->type = type;
    this->use = use;

    this->format = format;
    this->samples = sampleCount;
    this->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    switch(type) {
    case AttachmentType::Resolve:
	this->loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    case AttachmentType::Colour:
	imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	imageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	break;
    case AttachmentType::Depth:
	imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	imageUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	break;
    }
    switch(use) {
    case AttachmentUse::TransientAttachment:
	imageUsageFlags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    case AttachmentUse::Attachment:
	finalImageLayout = imageLayout;
	storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	break;
    case AttachmentUse::ShaderRead:
	storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	finalImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageUsageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
	break;
    case AttachmentUse::PresentSrc:
	storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	finalImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	break;
    }
}

VkAttachmentReference AttachmentDesc::getAttachmentReference() {
    VkAttachmentReference attachRef;
    attachRef.attachment = index;
    attachRef.layout = imageLayout;
    return attachRef;
}

VkAttachmentDescription AttachmentDesc::getAttachmentDescription() {
    VkAttachmentDescription desc;
    desc.flags = 0;
    desc.format = format;
    desc.samples = samples;
    desc.loadOp = loadOp;
    desc.storeOp = storeOp;
    desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    desc.finalLayout = finalImageLayout;
    return desc;
}

AttachmentType AttachmentDesc::getType() {
    return this->type;
}


RenderPass::RenderPass(VkDevice device, std::vector<AttachmentDesc> attachments) {
    this->attachmentDescription = attachments;
    this->device = device;

    std::vector<VkAttachmentDescription> attachDescVK(attachments.size());
    bool hasDepth, hasResolve;
    hasDepth = hasResolve = false;
    VkAttachmentReference depthRef;
    VkAttachmentReference resolveRef;
    std::vector<VkAttachmentReference> colourRefs;
    for(int i = 0; i < attachments.size(); i++) {
	attachDescVK[i] = attachments[i].getAttachmentDescription();
	VkAttachmentReference attachRef = attachments[i].getAttachmentReference();
	switch(attachments[i].getType()) {
	case AttachmentType::Colour:
	    colourRefs.push_back(attachRef);
	    break;
	case AttachmentType::Depth:
	    if(hasDepth)
		std::runtime_error("Render Pass State Error: Can only have 1 depth attachment");
	    hasDepth = true;
	    depthRef = attachRef;
	    break;
	case AttachmentType::Resolve:
	    if(hasResolve)
		std::runtime_error("Render Pass State Error: Can only have 1 resolve attachment");
	    hasResolve = true;
	    resolveRef = attachRef;
	}
    }

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = (uint32_t)colourRefs.size();
    subpass.pColorAttachments = colourRefs.data();
    if(hasDepth)
	subpass.pDepthStencilAttachment = &depthRef;
    if(hasResolve)
	subpass.pResolveAttachments = &resolveRef;

    VkRenderPassCreateInfo createInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    createInfo.attachmentCount = (uint32_t)attachDescVK.size();
    createInfo.pAttachments = attachDescVK.data();
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    //TODO subpass Dependancies:
    //createInfo.dependencyCount =
    //createInfo.pDependencies =
    throw std::runtime_error("Subpass Dependancies not set");

    vkCreateRenderPass(device, &createInfo, VK_NULL_HANDLE, &this->renderpass);
}

RenderPass::~RenderPass() {
    vkDestroyRenderPass(device, this->renderpass, VK_NULL_HANDLE);
}
