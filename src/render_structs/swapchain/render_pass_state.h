#ifndef SWAPCHAIN_RENDER_PASS_STATE_H
#define SWAPCHAIN_RENDER_PASS_STATE_H

#include <vector>
#include <volk.h>
#include "attachment_image.h"


#include <iostream>

//TODO make this more abstract, instead of just writing out the state

std::vector<AttachmentImageDescription> getOffscreenAttachmentImageDescriptions(VkSampleCountFlagBits samples, VkFormat swapchainFormat, VkFormat depthFormat) {
    std::vector<AttachmentImageDescription> attachments;
    uint32_t attachmentIndex = 0;

    AttachmentImageType type = samples == VK_SAMPLE_COUNT_1_BIT ?
	AttachmentImageType::Colour : AttachmentImageType::Resolve;
    AttachmentImageDescription resolve(attachmentIndex++, type);
    resolve.samples = VK_SAMPLE_COUNT_1_BIT;
    resolve.format = swapchainFormat;
    resolve.finalImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    resolve.imageUsageFlags  |= VK_IMAGE_USAGE_SAMPLED_BIT;
    //TODO check if this is nessecary
    if(resolve.type == AttachmentImageType::Resolve)
	resolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    else
	resolve.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    resolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments.push_back(resolve);
    
    
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
    depth.finalImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments.push_back(depth);

    return attachments;
}

std::vector<VkSubpassDependency> getOffscreenSubpassDependancies() {
    std::vector<VkSubpassDependency> dependencies(2);
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
    return dependencies;
}


std::vector<AttachmentImageDescription> getFinalAttachmentImageDescriptions(VkFormat swapchainFormat) {
    std::vector<AttachmentImageDescription> attachments;
    uint32_t attachmentIndex = 0;

    AttachmentImageDescription colour(attachmentIndex++, AttachmentImageType::Colour);
    colour.samples = VK_SAMPLE_COUNT_1_BIT;
    colour.format = swapchainFormat;
    colour.finalImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colour.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colour.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments.push_back(colour);
    
    return attachments;
}

std::vector<VkSubpassDependency> getFinalSubpassDependancies() {
    std::vector<VkSubpassDependency> dependencies(1);
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = 0;
    dependencies[0].dstSubpass = 0;
    dependencies[0].dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    return dependencies;
}


#endif
