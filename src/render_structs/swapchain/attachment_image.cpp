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
    std::cout << "mem offset: " << *pMemoryRequirements << std::endl;
    *pMemoryRequirements += vkhelper::correctMemoryAlignment(memReq.size, memReq.alignment);
    std::cout << "size to add: " << memReq.size << std::endl;
    std::cout << "alignment: " << memReq.alignment << std::endl;
    std::cout << "corrected: " << vkhelper::correctMemoryAlignment(memReq.size, memReq.alignment)
	      << std::endl;
    std::cout << "new mem offset: " << *pMemoryRequirements << std::endl << std::endl;
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
