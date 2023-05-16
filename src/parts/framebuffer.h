#ifndef PARTS_FRAMEBUFFER_H
#define PARTS_FRAMEBUFFER_H

#include <stdint.h>
#include <volk.h>
#include <vector>

namespace part {
    namespace create {
      VkResult Framebuffer(VkDevice device,
			   VkRenderPass renderPass,
			   VkFramebuffer *pFramebuffer,
			   std::vector<VkImageView> attachments,
			   uint32_t width, uint32_t height);
    }
}
 
#endif
