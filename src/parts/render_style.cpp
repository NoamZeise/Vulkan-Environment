#include "render_style.h"

#include <stdexcept>
#include <iostream>
#include <fstream>

#include "../logger.h"

namespace part
{
namespace create
{

VkShaderModule _loadShaderModule(VkDevice device, std::string file);

void GraphicsPipeline(
    VkDevice device, Pipeline *pipeline, VkSampleCountFlagBits  msaaSamples,
    VkRenderPass renderPass, std::vector<DS::DescriptorSet*> descriptorSets,
    std::vector<VkPushConstantRange> pushConstantsRanges,
    std::string vertexShaderPath, std::string fragmentShaderPath,
    bool useDepthTest, bool useMultisampling, bool useBlend, bool useSampleShading,
    VkExtent2D extent, VkCullModeFlags cullMode,
    std::vector<VkVertexInputAttributeDescription> vertexAttribDesc,
    std::vector<VkVertexInputBindingDescription> vertexBindingDesc) {

    
  pipeline->descriptorSets = descriptorSets;

  // load shader modules
  auto vertexShaderModule = _loadShaderModule(device, vertexShaderPath);
  auto fragmentShaderModule = _loadShaderModule(device, fragmentShaderPath);

  // create pipeline layout
  std::vector<VkDescriptorSetLayout> descriptorSetLayouts(
      descriptorSets.size());

  for (size_t i = 0; i < descriptorSets.size(); i++)
  {
      if(descriptorSets[i] != nullptr) {
	  descriptorSetLayouts[i] = descriptorSets[i]->layout;
      }
      else
	  throw std::runtime_error("Descriptor Set was null");
  }

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pipelineLayoutInfo.setLayoutCount =
      static_cast<uint32_t>(descriptorSetLayouts.size());
  pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
  pipelineLayoutInfo.pushConstantRangeCount =
      static_cast<uint32_t>(pushConstantsRanges.size());
  pipelineLayoutInfo.pPushConstantRanges = pushConstantsRanges.data();
  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                             &pipeline->layout) != VK_SUCCESS)
    throw std::runtime_error("failed to create pipeline layout");

  // config input assemby
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

  // config vertex input
  VkPipelineVertexInputStateCreateInfo vertexInputInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

  if (vertexAttribDesc.size() == 0 && vertexBindingDesc.size() == 0) {
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
  } else {
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(vertexAttribDesc.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttribDesc.data();
    vertexInputInfo.vertexBindingDescriptionCount =
        static_cast<uint32_t>(vertexBindingDesc.size());
    vertexInputInfo.pVertexBindingDescriptions = vertexBindingDesc.data();
  }

  // config viewport and scissor
  VkViewport viewport{
      0.0f,
      0.0f, // x  y
      (float)extent.width,
      (float)extent.height, // width  height
      0.0f,
      1.0f // min/max depth
  };
  VkRect2D scissor{VkOffset2D{0, 0}, extent};

  VkPipelineViewportStateCreateInfo viewportInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  viewportInfo.viewportCount = 1;
  viewportInfo.pViewports = &viewport;
  viewportInfo.scissorCount = 1;
  viewportInfo.pScissors = &scissor;

  // config vertex shader
  VkPipelineShaderStageCreateInfo vertexStageInfo{
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
  vertexStageInfo.module = vertexShaderModule;
  vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertexStageInfo.pName = "main";

  // config rasterization
  VkPipelineRasterizationStateCreateInfo rasterizationInfo{
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  rasterizationInfo.depthClampEnable = VK_FALSE;
  rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizationInfo.cullMode = cullMode;
  rasterizationInfo.lineWidth = 1.0f;
  rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  // fragment shader
  VkPipelineShaderStageCreateInfo fragmentStageInfo{
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
  fragmentStageInfo.module = fragmentShaderModule;
  fragmentStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragmentStageInfo.pName = "main";

  // config multisampler
  VkPipelineMultisampleStateCreateInfo multisampleInfo{
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};

  if (useMultisampling) {
      multisampleInfo.rasterizationSamples = msaaSamples;
    if (useSampleShading) {
      multisampleInfo.minSampleShading = 1.0f;
      multisampleInfo.sampleShadingEnable = VK_TRUE;
    }
  } else {
    multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  }

  // config depthStencil
  VkPipelineDepthStencilStateCreateInfo depthStencilInfo{
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  if (useDepthTest) {
    depthStencilInfo.depthTestEnable = VK_TRUE;
    depthStencilInfo.depthWriteEnable = VK_TRUE;
  } else {
    depthStencilInfo.depthTestEnable = VK_FALSE;
    depthStencilInfo.depthWriteEnable = VK_FALSE;
  }

  depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencilInfo.depthBoundsTestEnable = VK_FALSE;

  // config colour blend attachment
  VkPipelineColorBlendAttachmentState blendAttachment{};
  blendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  if (useBlend)
    blendAttachment.blendEnable = VK_TRUE;
  else
    blendAttachment.blendEnable = VK_FALSE;
  blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
  // config colour blend state
  VkPipelineColorBlendStateCreateInfo blendInfo{
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  blendInfo.logicOpEnable = VK_FALSE;
  blendInfo.attachmentCount = 1;
  blendInfo.pAttachments = &blendAttachment;

  // set dynamic states
  // std::array<VkDynamicState, 2> dynamicStates{ };

  VkPipelineDynamicStateCreateInfo dynamicStateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  dynamicStateInfo.dynamicStateCount = 0;
  dynamicStateInfo.pDynamicStates = nullptr;

  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
      vertexStageInfo, fragmentStageInfo};

  // create graphics pipeline
  VkGraphicsPipelineCreateInfo createInfo{
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  createInfo.layout = pipeline->layout;
  createInfo.renderPass = renderPass;
  createInfo.pViewportState = &viewportInfo;
  createInfo.pInputAssemblyState = &inputAssemblyInfo;
  createInfo.pVertexInputState = &vertexInputInfo;
  createInfo.pRasterizationState = &rasterizationInfo;
  createInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  createInfo.pStages = shaderStages.data();
  createInfo.pMultisampleState = &multisampleInfo;
  createInfo.pDepthStencilState = &depthStencilInfo;
  createInfo.pColorBlendState = &blendInfo;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr,
                                &pipeline->pipeline) != VK_SUCCESS)
    throw std::runtime_error("failed to create graphics pipelines!");

  // destory shader modules
  vkDestroyShaderModule(device, vertexShaderModule, nullptr);
  vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
}

//HELPERS


VkShaderModule _loadShaderModule(VkDevice device, std::string file)
{
	VkShaderModule shaderModule;

	std::ifstream in(file, std::ios::binary | std::ios::ate);
	if (!in.is_open())
		throw std::runtime_error("failed to load shader at " + file);

	size_t fileSize = (size_t)in.tellg();
	std::vector<char> shaderCode(fileSize);

	in.seekg(0);
	in.read(shaderCode.data(), fileSize);
	in.close();

	VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.codeSize = shaderCode.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		throw std::runtime_error("failed to create shader module from: " + file);

	return shaderModule;
}

}
}
