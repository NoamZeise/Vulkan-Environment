#ifndef PARTS_RENDER_STYLE_H
#define PARTS_RENDER_STYLE_H

#include "../pipeline.h"
#include "../render_structs/swapchain/swapchain.h"
#include "../shader_internal.h"
#include "framebuffer.h"
#include <volk.h>

#include <string>
#include <vector>

namespace part {
  namespace create {
    void GraphicsPipeline(VkDevice device,
			  Pipeline* pipeline, VkSampleCountFlagBits msaaSamples,
			  VkRenderPass renderPass,
			  std::vector<DS::DescriptorSet*> descriptorSets,
			  std::vector<VkPushConstantRange> pushConstantsRanges,
			  std::string vertexShaderPath, std::string fragmentShaderPath,
			  bool useDepthTest, bool useMultisampling, bool useBlend, bool useSampleShading,
			  VkExtent2D extent,
			  VkCullModeFlags cullMode,
			  std::vector<VkVertexInputAttributeDescription> vertexAttribDesc,
			  std::vector<VkVertexInputBindingDescription> vertexBindingDesc);

  }
}

#endif
