#ifndef PARTS_IMAGES_H
#define PARTS_IMAGES_H

#include "vulkan/vulkan_core.h"
#include <stdexcept>

namespace part
{
    namespace create
    {
        VkMemoryRequirements Image(VkDevice device, VkPhysicalDevice physicalDevice,
                                    VkImage* image, VkImageUsageFlags usageFlags,
                                    VkExtent2D extent, VkFormat format,
                                    VkSampleCountFlagBits sampleFlags);

        void ImageView(VkDevice device, VkImageView* imgView, VkImage image,
                              VkFormat format, VkImageAspectFlags aspectFlags);
    }
}

#endif
