#include "images.h"

namespace part
{
 namespace create
{

VkMemoryRequirements Image(VkDevice device, VkPhysicalDevice physicalDevice,
                                    VkImage* image, VkImageUsageFlags usageFlags,
                                    VkExtent2D extent, VkFormat format,
                                    VkSampleCountFlagBits sampleFlags)
{
//create attach image
	VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = extent.width;
	imageInfo.extent.height = extent.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usageFlags;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = sampleFlags;

	if (vkCreateImage(device, &imageInfo, nullptr, image) != VK_SUCCESS)
		throw std::runtime_error("failed to create attachment image");

	//assign memory for attach image
	VkMemoryRequirements memreq;
	vkGetImageMemoryRequirements(device, *image, &memreq);
	return memreq;
}

void ImageView(VkDevice device, VkImageView* imgView, VkImage image,
                              VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewInfo { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;

	if(vkCreateImageView(device, &viewInfo, nullptr, imgView) != VK_SUCCESS)
		throw std::runtime_error("Failed to create image view for attachment!");

}


}
}
