#ifndef TOOLS_RENDER_STYLE_H
#define TOOLS_RENDER_STYLE_H

#include "../render_structs.h"
#include "../pipeline.h"
#include "framebuffer.h"
#include <volk.h>

#include <string>
#include <vector>
#include <fstream>

namespace part
{
    namespace create
    {
        void RenderPass(VkDevice device, VkRenderPass* renderPass, SwapChain swapchain, bool presentOnly);
        void GraphicsPipeline(VkDevice device, Pipeline* pipeline, SwapChain swapchain, VkRenderPass renderPass,
								std::vector<DS::DescriptorSet*> descriptorSets,
								std::vector<VkPushConstantRange> pushConstantsRanges,
							  	std::string vertexShaderPath, std::string fragmentShaderPath,
								bool useDepthTest, bool useMultisampling, bool useBlend,
                                VkExtent2D extent, VkCullModeFlags cullMode,
								std::vector<VkVertexInputAttributeDescription> vertexAttribDesc,
								std::vector<VkVertexInputBindingDescription> vertexBindingDesc
        );

    }
}

#endif
