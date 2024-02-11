#ifndef PARTS_RENDER_STYLE_H
#define PARTS_RENDER_STYLE_H

#include "../pipeline.h"
#include "../shader_internal.h"
#include <volk.h>

#include <string>
#include <vector>

namespace part {
  namespace create {

    struct PipelineConfig {
	bool useDepthTest = true;
	bool useMultisampling;
	VkSampleCountFlagBits msaaSamples;
	bool useSampleShading;
	bool blendEnabled = true;
	VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
	VkBlendOp blendOp = VK_BLEND_OP_ADD;
    };
    
    void GraphicsPipeline(VkDevice device,
			  Pipeline* pipeline,
			  VkRenderPass renderPass,
			  std::vector<DS::DescriptorSet*> descriptorSets,
			  std::vector<VkPushConstantRange> pushConstantsRanges,
			  std::string vertexShaderPath, std::string fragmentShaderPath,
			  VkExtent2D extent,
			  std::vector<VkVertexInputAttributeDescription> vertexAttribDesc,
			  std::vector<VkVertexInputBindingDescription> vertexBindingDesc,
			  PipelineConfig config);

  }
}

#endif
