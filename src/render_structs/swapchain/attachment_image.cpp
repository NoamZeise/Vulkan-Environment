#include "attachment_image.h"

#include "../../parts/images.h"
#include "../../parts/part_macros.h"
#include "../../vkhelper.h"

void AttachmentImage::Destroy(VkDevice device)
{
    vkDestroyImageView(device, view, nullptr);
    vkDestroyImage(device, image, nullptr);
}


VkResult AttachmentImage::CreateImage(VkDevice device,
				      VkExtent2D extent,
				      VkDeviceSize *pMemoryRequirements,
				      uint32_t *pMemoryFlagBits)  {
    VkResult result = VK_SUCCESS;
    VkMemoryRequirements memReq;
    memoryOffset = *pMemoryRequirements;
    returnOnErr(part::create::Image(
			device,
			&image,
			&memReq,
			desc.imageUsageFlags,
			extent,
			desc.format,
			desc.samples, 1));
    
    *pMemoryRequirements += vkhelper::correctMemoryAlignment(memReq.size, memReq.alignment);
    *pMemoryFlagBits |= memReq.memoryTypeBits;
    return result;
}

VkResult AttachmentImage::CreateImageView(VkDevice device,
			 VkDeviceMemory attachmentMemory) {
    vkBindImageMemory(device, image,
		      attachmentMemory, memoryOffset);

    return part::create::ImageView(device,
				   &view,
				   image,
				   desc.format,
				   desc.imageAspectFlags);
}
