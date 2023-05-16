#ifndef PARTS_IMAGES_H
#define PARTS_IMAGES_H

#include <volk.h>
#include <stdexcept>

namespace part {
    namespace create {
        VkResult Image(VkDevice device,
		 VkImage* image, VkMemoryRequirements *pMemoryRequirements,
		 VkImageUsageFlags usageFlags,
		 VkExtent2D extent, VkFormat format,
		       VkSampleCountFlagBits sampleFlags, uint32_t mipLevels);

        VkResult ImageView(VkDevice device, VkImageView* imgView, VkImage image,
                              VkFormat format, VkImageAspectFlags aspectFlags);
    }
}

#endif
