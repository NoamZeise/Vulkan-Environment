#include "render_style.h"

#include <config.h>
#include <stdexcept>
#include <iostream>


namespace part
{
namespace create
{

VkShaderModule _loadShaderModule(VkDevice device, std::string file);

void RenderPass(VkDevice device, VkRenderPass *renderPass, SwapChain swapchain,
                bool presentOnly) {
  if (presentOnly) {
    // create attachments

    // present attachment
    VkAttachmentReference colourAttachmentRef{
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentDescription colourAttachment{};
    colourAttachment.format = swapchain.format.format;
    colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    std::vector<VkAttachmentDescription> attachments;
    attachments = {colourAttachment};
    // create subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colourAttachmentRef;

    std::array<VkSubpassDescription, 1> subpasses = {subpass};

    // depenancy to external events
    std::array<VkSubpassDependency, 1> dependencies;
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = 0;
    dependencies[0].dstSubpass = 0;
    dependencies[0].dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo createInfo{
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    createInfo.pAttachments = attachments.data();
    createInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
    createInfo.pSubpasses = subpasses.data();
    createInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    createInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(device, &createInfo, nullptr, renderPass) !=
        VK_SUCCESS)
      throw std::runtime_error("failed to create render pass!");
  } else {
    // create attachments

    // present attachment
    VkAttachmentReference colourAttachmentRef{
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentDescription colourAttachment{};
    colourAttachment.format = swapchain.format.format;
    colourAttachment.samples = swapchain.maxMsaaSamples;
    if (settings::MULTISAMPLING) {
      colourAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    } else {
      colourAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    if (settings::MULTISAMPLING)
      colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // depth attachment
    VkAttachmentReference depthBufferRef{
        1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = swapchain.frameData[0].depthBuffer.format;
    depthAttachment.samples = swapchain.maxMsaaSamples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // resolve attachment
    VkAttachmentReference resolveAttachmentRef{
        2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentDescription resolveAttachment{};
    resolveAttachment.format = swapchain.format.format;
    resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    std::vector<VkAttachmentDescription> attachments;
    if (settings::MULTISAMPLING)
      attachments = {colourAttachment, depthAttachment, resolveAttachment};
    else
      attachments = {colourAttachment, depthAttachment};
    // create subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colourAttachmentRef;
    if (settings::MULTISAMPLING)
      subpass.pResolveAttachments = &resolveAttachmentRef;
    subpass.pDepthStencilAttachment = &depthBufferRef;

    std::array<VkSubpassDescription, 1> subpasses = {subpass};

    // depenancy to external events
    std::array<VkSubpassDependency, 2> dependencies;
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = 0;
    dependencies[0].dstSubpass = 0;
    dependencies[0].dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[1].srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo createInfo{
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    createInfo.pAttachments = attachments.data();
    createInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
    createInfo.pSubpasses = subpasses.data();
    createInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    createInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(device, &createInfo, nullptr, renderPass) !=
        VK_SUCCESS)
      throw std::runtime_error("failed to create render pass!");
  }
}

void Framebuffer(VkDevice device, VkRenderPass renderPass,
                 VkFramebuffer *framebuffer,
                 std::vector<VkImageView> attachments, uint32_t width,
                 uint32_t height) {
  VkFramebufferCreateInfo createInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
  createInfo.renderPass = renderPass;
  createInfo.width = width;
  createInfo.height = height;
  createInfo.layers = 1;
  createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  createInfo.pAttachments = attachments.data();

  if (vkCreateFramebuffer(device, &createInfo, nullptr, framebuffer) !=
      VK_SUCCESS)
    throw std::runtime_error("failed to create framebuffer");
}

void GraphicsPipeline(
    VkDevice device, Pipeline *pipeline, SwapChain swapchain,
    VkRenderPass renderPass, std::vector<DS::DescriptorSet*> descriptorSets,
    std::vector<VkPushConstantRange> pushConstantsRanges,
    std::string vertexShaderPath, std::string fragmentShaderPath,
    bool useDepthTest, bool useMultisampling, bool useBlend, VkExtent2D extent,
    VkCullModeFlags cullMode,
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
      if(descriptorSets[i] != nullptr)
	  descriptorSetLayouts[i] = descriptorSets[i]->layout;
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
    multisampleInfo.rasterizationSamples = swapchain.maxMsaaSamples;
    if (settings::SAMPLE_SHADING) {
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
