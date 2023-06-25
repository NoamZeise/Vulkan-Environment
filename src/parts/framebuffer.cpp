#include "framebuffer.h"

namespace part {
namespace create {

  //TODO: remove after change to new renderpass/swapchain system
VkResult Framebuffer(VkDevice device, VkRenderPass renderPass,
                 VkFramebuffer *pFramebuffer,
                 std::vector<VkImageView> attachments, uint32_t width, uint32_t height) {
  VkFramebufferCreateInfo createInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
  createInfo.renderPass = renderPass;
  createInfo.width = width;
  createInfo.height = height;
  createInfo.layers = 1;
  createInfo.attachmentCount = (uint32_t)attachments.size();
  createInfo.pAttachments = attachments.data();

  return vkCreateFramebuffer(device, &createInfo, nullptr, pFramebuffer);
}


} // namespace create
} // namespace part
