#include "renderpass.h"

#include <stdexcept>
#include "logger.h"
#include "parts/images.h"
#include "vkhelper.h"

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
	//must be depth_stencil, unless seperate depth stencil layout feature is enabled.
	imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
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

AttachmentType AttachmentDesc::getType() { return this->type; }

AttachmentUse AttachmentDesc::getUse() { return this->use; }

void AttachmentDesc::getImageProps(VkFormat *imageFormat,
				   VkImageUsageFlags *imageUsage,
				   VkImageAspectFlags *imageAspect,
				   VkSampleCountFlagBits *sampleCount) {
    *imageFormat = this->format;
    *imageUsage = this->imageUsageFlags;
    *imageAspect = this->imageAspectFlags;
    *sampleCount = this->samples;
}

class AttachmentImage {
public:
    AttachmentImage(AttachmentDesc &desc);
    void Destroy(VkDevice device);
    VkResult CreateImage(VkDevice device,
			 VkExtent2D extent,
			 VkDeviceSize *pMemoryRequirements,
			 uint32_t *pMemoryFlagBits);
    void AddImage(VkImage);
    VkResult CreateImageView(VkDevice device,
			     VkDeviceMemory attachmentMemory);
    VkImageView getView();
    bool isUsingExternalImage();

private:
    enum class state {
	unmade,
	image,
	imageview,
    };

    state state = state::unmade;

    bool usingExternalImage = false;
    VkImage image;
    VkImageView view;
    size_t memoryOffset;

    VkFormat imageFormat;
    VkImageUsageFlags imageUsage;
    VkImageAspectFlags imageAspect;
    VkSampleCountFlagBits sampleCount;
};

AttachmentImage::AttachmentImage(AttachmentDesc &attachmentDesc) {
    attachmentDesc.getImageProps(&imageFormat, &imageUsage, &imageAspect, &sampleCount);
    if(attachmentDesc.getUse() == AttachmentUse::PresentSrc)
	usingExternalImage = true;
}

void AttachmentImage::Destroy(VkDevice device)
{
    switch(state) {
    case state::imageview:
	vkDestroyImageView(device, view, nullptr);
    case state::image:
	if(!usingExternalImage) 
	    vkDestroyImage(device, image, nullptr);
    }
}

VkResult AttachmentImage::CreateImage(VkDevice device,
				      VkExtent2D extent,
				      VkDeviceSize *pMemoryRequirements,
				      uint32_t *pMemoryFlagBits)  {
    if(state != state::unmade)
	throw std::runtime_error("Error: invalid attachment image state, "
				 "tried to create image but previous "
				 "state wasn't state::unmade");
    if(usingExternalImage)
	throw std::runtime_error("Attachment Image Error: invalid attachment image op, "
				 "tried to create image with attachment that "
				 "uses an external image.");
    VkResult result = VK_SUCCESS;
    VkMemoryRequirements memReq;
    returnOnErr(part::create::Image(
			device,
			&image,
			&memReq,
			imageUsage,
			extent,
			imageFormat,
			sampleCount, 1));
    *pMemoryRequirements = vkhelper::correctMemoryAlignment(*pMemoryRequirements, memReq.alignment);
    this->memoryOffset = *pMemoryRequirements;
    *pMemoryRequirements += vkhelper::correctMemoryAlignment(memReq.size, memReq.alignment);
    *pMemoryFlagBits |= memReq.memoryTypeBits;
    
    if(result==VK_SUCCESS)
	state = state::image;
    return result;
}

void AttachmentImage::AddImage(VkImage image) {
    if(state != state::unmade)
	throw std::runtime_error("Attachment Image Error: invalid attachment image state, "
				 "tried to add image but previous "
				 "state wasn't state::unmade");
    if(!usingExternalImage)
	throw std::runtime_error("Attachment Image Error: invalid attachment image op, "
				 "tried to add image to attachment that "
				 "does not use an external image (ie swapchain image)");
    this->image = image;
    state = state::image;
}

VkResult AttachmentImage::CreateImageView(VkDevice device,
			 VkDeviceMemory attachmentMemory) {
    if(state != state::image)
	throw std::runtime_error("Attachment Image Error: "
				 "invalid attachment image state, tried to "
				 "create image view, but previous state "
				 "wasn't state::image");
    if(!usingExternalImage)
	vkBindImageMemory(device, image,
			  attachmentMemory, memoryOffset);
    
    VkResult result = part::create::ImageView(device,
					      &view,
					      image,
					      imageFormat,
					      imageAspect);
    if(result == VK_SUCCESS)
	state = state::imageview;
    return result;
}

VkImageView AttachmentImage::getView() { return this->view; }

bool AttachmentImage::isUsingExternalImage() { return this->usingExternalImage; }

Framebuffer::~Framebuffer() {
    if(framebufferCreated)
	vkDestroyFramebuffer(device, framebuffer, VK_NULL_HANDLE);
    for(auto &a: attachments)
	a.Destroy(device);
}


enum class SubpassDependancyType {
  PreviousImageOps,
  FutureShaderRead,
};

VkSubpassDependency genSubpassDependancy(bool colour, bool depth,
                                         SubpassDependancyType depType);

RenderPass::RenderPass(VkDevice device, std::vector<AttachmentDesc> attachments,
	       float clearColour[3]) {
    this->attachmentDescription.resize(attachments.size());
    this->device = device;

    std::vector<VkAttachmentDescription> attachDescVK(attachments.size());
    bool hasDepth, hasResolve, hasShaderReadAttachment;
    hasDepth = hasResolve = hasShaderReadAttachment = false;
    VkAttachmentReference depthRef;
    VkAttachmentReference resolveRef;
    std::vector<VkAttachmentReference> colourRefs;
    for(int i = 0; i < attachments.size(); i++) {
	VkClearValue clear;
	if(attachments[i].getIndex() > attachments.size())
	    throw std::runtime_error("Render Pass Creation Error: Attachment Index "
				     "was greater than the number of supplied attachments");
	if(attachmentDescription[attachments[i].getIndex()].wasCreated())
	    throw std::runtime_error("Render Pass Creation Error: tried to have two attachments "
				     "with the same index!");
	attachmentDescription[attachments[i].getIndex()] = attachments[i];    
	attachDescVK[attachments[i].getIndex()] = attachments[i].getAttachmentDescription();

	VkAttachmentReference attachRef = attachments[i].getAttachmentReference();
	if(attachments[i].getUse() == AttachmentUse::ShaderRead)
	    hasShaderReadAttachment = true;
	switch(attachments[i].getType()) {
	case AttachmentType::Colour:
	    colourRefs.push_back(attachRef);
	    clear.color = {{clearColour[0], clearColour[1], clearColour[2], 1.0f}};
	    attachmentClears.push_back(clear);
	    break;
	case AttachmentType::Depth:
	    if(hasDepth)
		std::runtime_error("Render Pass State Error: Can only have 1 depth attachment");
	    hasDepth = true;
	    depthRef = attachRef;
	    clear.depthStencil = {1.0f, 0};
	    attachmentClears.push_back(clear);
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

    std::vector<VkSubpassDependency> subpassDependancies;
    subpassDependancies.push_back(
	    genSubpassDependancy(colourRefs.size() > 0, hasDepth,
				 SubpassDependancyType::PreviousImageOps));
    if(hasShaderReadAttachment)
	subpassDependancies.push_back(
		genSubpassDependancy(colourRefs.size() > 0, hasDepth,
				     SubpassDependancyType::FutureShaderRead));

    VkRenderPassCreateInfo createInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    createInfo.attachmentCount = (uint32_t)attachDescVK.size();
    createInfo.pAttachments = attachDescVK.data();
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = subpassDependancies.size();
    createInfo.pDependencies = subpassDependancies.data();
    VkResult res = vkCreateRenderPass(device, &createInfo, VK_NULL_HANDLE, &this->renderpass);
    checkResultAndThrow(res, "Failed to created render pass!");
}

RenderPass::~RenderPass() {
    framebuffers.clear();
    vkDestroyRenderPass(device, this->renderpass, VK_NULL_HANDLE);
}

VkResult RenderPass::createFramebufferImages(std::vector<VkImage> *swapchainImages,
					 VkExtent2D extent,
					 VkDeviceSize *pMemSize,
					 uint32_t *pMemFlags) {
    VkResult result = VK_SUCCESS;
    std::vector<AttachmentImage> attachImages;
    for(int i = 0; i < attachmentDescription.size(); i++)
	attachImages.push_back(AttachmentImage(attachmentDescription[i]));
    framebuffers.clear();
    framebuffers.resize((swapchainImages->size()));
    framebufferExtent = extent;

    VkDeviceSize imageMemReqs;
    for(int i = 0; i < framebuffers.size(); i++) {
	framebuffers[i].attachments = attachImages;
	framebuffers[i].device = this->device;
	for(AttachmentImage &im: framebuffers[i].attachments) {
	    if(im.isUsingExternalImage())
		im.AddImage(swapchainImages->at(i));
	    else {
		msgAndReturnOnErr(
			im.CreateImage(device, extent, pMemSize, pMemFlags),
			"RenderPass Error: Failed to create framebuffer attachment image");
	    }
	}   
    }
    return result;
}

VkResult RenderPass::createFramebuffers(VkDeviceMemory framebufferImageMemory) {
    VkResult result = VK_SUCCESS;
    VkFramebufferCreateInfo fbCreateInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fbCreateInfo.renderPass = renderpass;
    fbCreateInfo.attachmentCount = attachmentDescription.size();
    fbCreateInfo.flags = 0;
    fbCreateInfo.width = framebufferExtent.width;
    fbCreateInfo.height = framebufferExtent.height;
    fbCreateInfo.layers = 1;
    for(auto& fb: framebuffers) {
	std::vector<VkImageView> attachViews(fb.attachments.size());
	for(int i = 0; i < attachViews.size(); i++) {
	    msgAndReturnOnErr(fb.attachments[i].CreateImageView(device, framebufferImageMemory),
			      "RenderPass Error: Failed to create image view for framebuffer");
	    attachViews[i] = fb.attachments[i].getView();
	}
	fbCreateInfo.pAttachments = attachViews.data();
	msgAndReturnOnErr(vkCreateFramebuffer(device, &fbCreateInfo,
					      VK_NULL_HANDLE, &fb.framebuffer),
			  "RenderPass Error: Failed to create Framebuffer");
	fb.framebufferCreated = true;
    }

    return result;
}

VkViewport fbViewport(VkExtent2D extent);
VkRect2D fbScissor(VkExtent2D extent);

void RenderPass::beginRenderPass(VkCommandBuffer cmdBuff, uint32_t frameIndex) {
    VkRenderPassBeginInfo beginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    if(frameIndex > framebuffers.size())
	throw std::runtime_error("RenderPass Error: Tried to start render pass, "
				 "but the frame index was out of range. "
				 "Ensure that thr renderpass framebuffers "
				 "and attachments have been created.");
    beginInfo.renderPass = renderpass;
    beginInfo.framebuffer = framebuffers[frameIndex].framebuffer;
    beginInfo.renderArea.offset = offset;
    beginInfo.renderArea.extent = framebufferExtent;
    beginInfo.clearValueCount = attachmentClears.size();
    beginInfo.pClearValues = attachmentClears.data();
    vkCmdBeginRenderPass(cmdBuff, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    VkViewport viewport = fbViewport(framebufferExtent);
    vkCmdSetViewport(cmdBuff, 0, 1, &viewport);
    VkRect2D scissor = fbScissor(framebufferExtent);
    vkCmdSetScissor(cmdBuff, 0, 1, &scissor);
}

std::vector<VkImageView> RenderPass::getAttachmentViews(uint32_t attachmentIndex) {
    if(framebuffers.empty())
	throw std::runtime_error("RenderPass Error: Tried to get attachment views but framebuffers"
				 " haven't been created.");
    if(attachmentDescription.size() <= attachmentIndex)
	throw std::runtime_error("RenderPass Error: Tried to get attchment views, but the"
				 " supplied attachment Index was out of range");
    if(attachmentDescription[attachmentIndex].getUse() != AttachmentUse::ShaderRead)
	throw std::runtime_error("RenderPass Error: Tried to get attachment views, but the"
				 " attachment is not for reading from a shader");

    std::vector<VkImageView> views(framebuffers.size());
    for(int i = 0; i < framebuffers.size(); i++)
	views[i] = framebuffers[i].attachments[attachmentIndex].getView();
    return views;
}

VkExtent2D RenderPass::getExtent() { return this->framebufferExtent; }

VkRenderPass RenderPass::getRenderPass() { return this->renderpass; }



VkViewport fbViewport(VkExtent2D extent) {
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    return viewport;
}

VkRect2D fbScissor(VkExtent2D extent) {
    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = extent;
    return scissor;
}

void setStageMask(VkPipelineStageFlags *pStageMask, bool colour, bool depth) {
    *pStageMask = 0;
    if(colour)
	*pStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    if(depth)
	*pStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
}

void setAccessMask(VkAccessFlags *pAccessMask, bool colour, bool depth) {
    *pAccessMask = 0;
    if(colour)
	*pAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    if(depth)
	*pAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
}

VkSubpassDependency genSubpassDependancy(bool colour, bool depth, SubpassDependancyType depType) {
    VkSubpassDependency dep;
    dep.srcStageMask = 0;
    dep.srcAccessMask = 0;
    dep.dstStageMask = 0;
    dep.dstAccessMask = 0;
    dep.dependencyFlags = 0;
    //dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT; ??? TODO: check if needed
    switch(depType) {
    case SubpassDependancyType::PreviousImageOps:
	dep.srcSubpass = VK_SUBPASS_EXTERNAL;
	dep.dstSubpass = 0;
	setStageMask(&dep.srcStageMask, colour, depth);
	setStageMask(&dep.dstStageMask, colour, depth);
	setAccessMask(&dep.dstAccessMask, colour, depth);
	break;
    case SubpassDependancyType::FutureShaderRead:
	dep.srcSubpass = 0;
	dep.dstSubpass = VK_SUBPASS_EXTERNAL;
	setStageMask(&dep.srcStageMask, colour, depth);
	setAccessMask(&dep.srcAccessMask, colour, depth);
	dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dep.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	break;
    }
    return dep;
}
