#ifndef VK_ENV_RENDER_PASS
#define VK_ENV_RENDER_PASS

#include <volk.h>
#include <vector>

enum class AttachmentType {
  Colour,
  Depth,
  Resolve,
};

enum class AttachmentUse {
  Attachment,
  TransientAttachment, 
  ShaderRead,
  PresentSrc,
};

class AttachmentDesc {
 public:
    AttachmentDesc(uint32_t index, AttachmentType type, AttachmentUse use,
		   VkSampleCountFlagBits sampleCount, VkFormat format);
    VkAttachmentReference getAttachmentReference();
    VkAttachmentDescription getAttachmentDescription();
    AttachmentType getType();
private:
    uint32_t index;
    AttachmentType type;
    AttachmentUse use;
    VkImageUsageFlags imageUsageFlags;
    VkImageAspectFlags imageAspectFlags;
    VkFormat format;
    VkSampleCountFlagBits samples;
    VkImageLayout imageLayout;
    VkImageLayout finalImageLayout;
    VkAttachmentLoadOp loadOp;
    VkAttachmentStoreOp storeOp;
};


class RenderPass {
 public:
    RenderPass(VkDevice device, std::vector<AttachmentDesc> attachments);
    ~RenderPass();

 private:
    VkDevice device;
    VkRenderPass renderpass;
    std::vector<AttachmentDesc> attachmentDescription;
};

#endif
