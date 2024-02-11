#include "render_style.h"

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <array>
#include "../logger.h"

namespace part
{
namespace create
{
  
  VkPipelineLayout createPipelineLayout(
	  VkDevice device,
	  std::vector<VkPushConstantRange> &pushConsts,
	  std::vector<DS::DescriptorSet*> &descSets);

  VkPipelineShaderStageCreateInfo shaderStageInfo(
	  VkShaderModule module, VkShaderStageFlagBits stage);
  
  VkShaderModule _loadShaderModule(VkDevice device, std::string file);

  void GraphicsPipeline(
	  VkDevice device, Pipeline *pipeline,
	  VkRenderPass renderPass, std::vector<DS::DescriptorSet*> descriptorSets,
	  std::vector<VkPushConstantRange> pushConstantsRanges,
	  std::string vertexShaderPath, std::string fragmentShaderPath,
	  VkExtent2D extent,
	  std::vector<VkVertexInputAttributeDescription> vertexAttribDesc,
	  std::vector<VkVertexInputBindingDescription> vertexBindingDesc,
	  PipelineConfig config) {

      // load shader modules
      VkPipelineLayout layout = createPipelineLayout(device, pushConstantsRanges, descriptorSets);

      // config input assemby
      VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
	  VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
      inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

      // config vertex input
      VkPipelineVertexInputStateCreateInfo vertexInputInfo{
	  VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
      vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)vertexAttribDesc.size();
      vertexInputInfo.pVertexAttributeDescriptions = vertexAttribDesc.data();
      vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)vertexBindingDesc.size();
      vertexInputInfo.pVertexBindingDescriptions = vertexBindingDesc.data();

      // config viewport and scissor
      VkViewport viewport{
	  0.0f, 0.0f, //x, y
	  (float)extent.width, (float)extent.height, // width, height
	  0.0f, 1.0f // min, max depth
      };
      VkRect2D scissor{VkOffset2D{0, 0}, extent};
      VkPipelineViewportStateCreateInfo viewportInfo{
	  VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
      viewportInfo.viewportCount = 1;
      viewportInfo.pViewports = &viewport;
      viewportInfo.scissorCount = 1;
      viewportInfo.pScissors = &scissor;

      // config rasterization
      VkPipelineRasterizationStateCreateInfo rasterizationInfo{
	  VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
      rasterizationInfo.depthClampEnable = VK_FALSE;
      rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
      rasterizationInfo.cullMode = config.cullMode;
      rasterizationInfo.lineWidth = 1.0f;
      rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

      // config multisampler
      VkPipelineMultisampleStateCreateInfo multisampleInfo{
	  VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};

      if (config.useMultisampling) {
	  multisampleInfo.rasterizationSamples = config.msaaSamples;
	  if (config.useSampleShading) {
	      multisampleInfo.minSampleShading = 1.0f;
	      multisampleInfo.sampleShadingEnable = VK_TRUE;
	  }
      } else {
	  multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
      }

      // config depthStencil
      VkPipelineDepthStencilStateCreateInfo depthStencilInfo{
	  VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
      if (config.useDepthTest) {
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
      if (config.blendEnabled)
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
      VkPipelineDynamicStateCreateInfo dynamicStateInfo{
	  VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
      dynamicStateInfo.dynamicStateCount = 0;
      dynamicStateInfo.pDynamicStates = nullptr;

      auto vertexShaderModule = _loadShaderModule(device, vertexShaderPath);
      auto fragmentShaderModule = _loadShaderModule(device, fragmentShaderPath);
      std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
	  shaderStageInfo(vertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT),
	  shaderStageInfo(fragmentShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT)};

      // create graphics pipeline
      VkPipeline vkpipeline;
      VkGraphicsPipelineCreateInfo createInfo{
	  VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
      createInfo.layout = layout;
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
				    &vkpipeline) != VK_SUCCESS)
	  throw std::runtime_error("failed to create graphics pipelines!");

      *pipeline = Pipeline(layout, vkpipeline, descriptorSets);
  
      // destory shader modules
      vkDestroyShaderModule(device, vertexShaderModule, nullptr);
      vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
  }


  
  // --- HELPERS ---

    VkPipelineLayout createPipelineLayout(
	  VkDevice device,
	  std::vector<VkPushConstantRange> &pushConsts,
	  std::vector<DS::DescriptorSet*> &descSets) {
      std::vector<VkDescriptorSetLayout> dslayout(descSets.size());
      for(int i = 0; i < dslayout.size(); i++) {
	  if(descSets[i] == nullptr)
	      throw std::runtime_error("Descriptor Set was null in createPipelineLayout");
	  dslayout[i] = descSets[i]->layout;
      }
  
      VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
      pipelineLayoutInfo.setLayoutCount =
	  static_cast<uint32_t>(dslayout.size());
      pipelineLayoutInfo.pSetLayouts = dslayout.data();
      pipelineLayoutInfo.pushConstantRangeCount =
	  static_cast<uint32_t>(pushConsts.size());
      pipelineLayoutInfo.pPushConstantRanges = pushConsts.data();

      VkPipelineLayout layout;
      if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
				 &layout) != VK_SUCCESS)
	  throw std::runtime_error("failed to create pipeline layout");
      return layout;
  }

    VkPipelineShaderStageCreateInfo shaderStageInfo(
	  VkShaderModule module, VkShaderStageFlagBits stage) {
      const char* SHADER_ENTRY_POINT = "main";
      VkPipelineShaderStageCreateInfo stageInfo {
	  VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
      stageInfo.module = module;
      stageInfo.stage = stage;
      stageInfo.pName = SHADER_ENTRY_POINT;
      return stageInfo;
  }

  VkShaderModule _loadShaderModule(VkDevice device, std::string file) {
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
