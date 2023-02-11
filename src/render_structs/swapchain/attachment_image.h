#ifndef RENDER_STRUCT_ATTACHMENT_IMAGE_H
#define RENDER_STRUCT_ATTACHMENT_IMAGE_H

#include <volk.h>

enum class AttachmentImageType {
    Colour,
    Depth,
    Resolve,
};

/// assuming this is not a stencil attachment
struct AttachmentImageDescription {
    AttachmentImageDescription()  {}
    /// just sets image usage/aspect and index and imageLayout during subpass.
    /// need to set the rest of the members manually
    AttachmentImageDescription(uint32_t attachmentIndex, AttachmentImageType type) {
	this->attachment_number = attachmentIndex;
	this->type = type;
        switch(type) {
	case AttachmentImageType::Resolve:
	case AttachmentImageType::Colour:
	    imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	    imageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	    imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	    break;
	case AttachmentImageType::Depth:
	    imageUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	    imageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	    imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	    break;
	}
    }
    AttachmentImageType type;
    VkImageUsageFlags imageUsageFlags;
    VkImageAspectFlags imageAspectFlags;
    
    uint32_t attachment_number;
    VkFormat format;
    VkSampleCountFlagBits samples;
    VkImageLayout imageLayout;
    VkImageLayout finalImageLayout;
    VkAttachmentLoadOp loadOp;
    VkAttachmentStoreOp storeOp;

    VkAttachmentReference getAttachmentReference() {
	VkAttachmentReference attachRef;
	attachRef.attachment = attachment_number;
	attachRef.layout = imageLayout;
	return attachRef;
    }
    
    VkAttachmentDescription getAttachmentDescription()  {
	VkAttachmentDescription attachDesc;
	attachDesc.flags = 0;
	attachDesc.format = format;
	attachDesc.samples = samples;
	attachDesc.loadOp = loadOp;
	attachDesc.storeOp = storeOp;
	attachDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachDesc.finalLayout = finalImageLayout;
	
	return attachDesc;
    }
};


struct AttachmentImage {
    AttachmentImage(AttachmentImageDescription attachmentDesc) {
        desc = attachmentDesc;
    }
    
    VkImage image;
    VkImageView view;
    size_t memoryOffset;
    
    AttachmentImageDescription desc;

    void Destroy(VkDevice device);

    VkResult CreateImage(VkDevice device,
			 VkExtent2D extent,
			 VkDeviceSize *pMemoryRequirements,
			 uint32_t *pMemoryFlagBits);

    VkResult CreateImageView(VkDevice device,
			     VkDeviceMemory attachmentMemory);
};


#endif
