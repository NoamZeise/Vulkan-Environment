#include "render_pass.h"

#include "part_macros.h"

#include <array>
#include <vector>


namespace part {
  namespace create {

    VkResult RenderPass(VkDevice device, VkRenderPass *renderPass,
			VkFormat swapchainFormat,
			VkFormat depthBufferFormat,
			VkSampleCountFlagBits msaaSamples,
			bool presentOnly) {
	VkResult result = VK_SUCCESS;
	if (presentOnly) {
	    // create attachments

	    // present attachment
	    VkAttachmentReference colourAttachmentRef{
		0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
	    VkAttachmentDescription colourAttachment{};
	    colourAttachment.format = swapchainFormat;
	    colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	    colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	    colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	    colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	    colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	    colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	    colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	    std::vector<VkAttachmentDescription> attachments;
	    attachments = {colourAttachment};
	    // create subpass
	    VkSubpassDescription subpass{};
	    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	    subpass.colorAttachmentCount = 1;
	    subpass.pColorAttachments = &colourAttachmentRef;

	    std::array<VkSubpassDescription, 1> subpasses = {subpass};

	    // depenancy to external events
	    std::array<VkSubpassDependency, 1> dependencies;
	    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	    dependencies[0].srcStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	    dependencies[0].srcAccessMask = 0;
	    dependencies[0].dstSubpass = 0;
	    dependencies[0].dstStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	    VkRenderPassCreateInfo createInfo{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
	    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	    createInfo.pAttachments = attachments.data();
	    createInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
	    createInfo.pSubpasses = subpasses.data();
	    createInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	    createInfo.pDependencies = dependencies.data();

	    returnOnErr(vkCreateRenderPass(device, &createInfo, nullptr, renderPass));
	} else {
	    // create attachments

	    // present attachment
	    VkAttachmentReference colourAttachmentRef{
		0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
	    VkAttachmentDescription colourAttachment{};
	    colourAttachment.format = swapchainFormat;
	    colourAttachment.samples = msaaSamples;
	    if (msaaSamples != VK_SAMPLE_COUNT_1_BIT) {
		colourAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	    } else {
		colourAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	    }
	    colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	    colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	    if (msaaSamples != VK_SAMPLE_COUNT_1_BIT)
		colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	    colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	    colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	    colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	    // depth attachment
	    VkAttachmentReference depthBufferRef{
		1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
	    VkAttachmentDescription depthAttachment{};
	    depthAttachment.format = depthBufferFormat;
	    depthAttachment.samples = msaaSamples;
	    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	    depthAttachment.finalLayout =
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	    // resolve attachment
	    VkAttachmentReference resolveAttachmentRef{
		2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
	    VkAttachmentDescription resolveAttachment{};
	    resolveAttachment.format = swapchainFormat;
	    resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	    resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	    resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	    resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	    resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	    resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	    resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	    std::vector<VkAttachmentDescription> attachments;
	    if (msaaSamples != VK_SAMPLE_COUNT_1_BIT)
		attachments = {colourAttachment, depthAttachment, resolveAttachment};
	    else
		attachments = {colourAttachment, depthAttachment};
	    // create subpass
	    VkSubpassDescription subpass{};
	    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	    subpass.colorAttachmentCount = 1;
	    subpass.pColorAttachments = &colourAttachmentRef;
	    if (msaaSamples != VK_SAMPLE_COUNT_1_BIT)
		subpass.pResolveAttachments = &resolveAttachmentRef;
	    subpass.pDepthStencilAttachment = &depthBufferRef;

	    std::array<VkSubpassDescription, 1> subpasses = {subpass};

	    // depenancy to external events
	    std::array<VkSubpassDependency, 2> dependencies;
	    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	    dependencies[0].srcStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	    dependencies[0].srcAccessMask = 0;
	    dependencies[0].dstSubpass = 0;
	    dependencies[0].dstStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	    dependencies[0].dstAccessMask =
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	    dependencies[1].srcSubpass = 0;
	    dependencies[1].srcStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	    dependencies[1].srcAccessMask =
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	    VkRenderPassCreateInfo createInfo{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
	    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	    createInfo.pAttachments = attachments.data();
	    createInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
	    createInfo.pSubpasses = subpasses.data();
	    createInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	    createInfo.pDependencies = dependencies.data();

	    returnOnErr(vkCreateRenderPass(device, &createInfo, nullptr, renderPass));
	}
	return result;
    }
  }
}
  
  
