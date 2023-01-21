#include "render.h"

#include "render_structs.h"
#include "descriptor_structs.h"
#include "parts/render_style.h"
#include "resources/model_render.h"
#include "resources/font_loader.h"
#include "resources/texture_loader.h"
#include "parts/primary.h"
#include "parts/swapchain.h"
#include "parts/descriptors.h"
#include "pipeline.h"


#include <GLFW/glfw3.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>

#include <glmhelper.h>

namespace vkenv {


bool Render::LoadVulkan()
{
    if(volkInitialize() != VK_SUCCESS) {
      return false;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    return true;
}

void checkVolk() {
	if(volkGetInstanceVersion() == 0) {
	    throw std::runtime_error("Vulkan has not been loaded! make sure Render::LoadVulkan has been called and checked before creating an instance of Render");
	}
 }

Render::Render(GLFWwindow *window)
{
    checkVolk();
  _initRender(window);
  _forceTargetResolution = false;
  _targetResolution = glm::vec2(_swapchain.swapchainExtent.width,
                                _swapchain.swapchainExtent.height);
}

Render::Render(GLFWwindow *window, glm::vec2 target)
{
    checkVolk();
  _initRender(window);
  _forceTargetResolution = true;
  _targetResolution = target;
}
void Render::_initRender(GLFWwindow *window)
{
  _window = window;
  part::create::Instance(&_instance);
#ifndef NDEBUG
  part::create::DebugMessenger(_instance, &_debugMessenger);
#endif

  if (glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS)
  {
      throw std::runtime_error("failed to create window surface!");
  }
  part::create::Device(_instance, &_base, _surface);

  // create general command pool
  VkCommandPoolCreateInfo commandPoolInfo{
      VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  commandPoolInfo.queueFamilyIndex = _base.queue.graphicsPresentFamilyIndex;
  if (vkCreateCommandPool(_base.device, &commandPoolInfo, nullptr, &_generalCommandPool) != VK_SUCCESS)
    throw std::runtime_error("failed to create command pool");

  // create transfer command buffer
  VkCommandBufferAllocateInfo commandBufferInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  commandBufferInfo.commandPool = _generalCommandPool;
  commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  commandBufferInfo.commandBufferCount = 1;
  if (vkAllocateCommandBuffers(_base.device, &commandBufferInfo, &_transferCommandBuffer))
    throw std::runtime_error("failed to allocate command buffer");

  _initStagingResourceManagers();
}

void Render::_initStagingResourceManagers() {
  _stagingModelLoader = new Resource::ModelRender(_base, _generalCommandPool);
  _stagingTextureLoader = new Resource::TextureLoader(_base, _generalCommandPool);
  _stagingFontLoader = new Resource::FontLoader();
  _stagingTextureLoader->LoadTexture("textures/error.png");
}

Render::~Render()
{
  vkQueueWaitIdle(_base.queue.graphicsPresentQueue);

  delete _textureLoader;
  delete _stagingTextureLoader;
  delete _modelLoader;
  delete _stagingModelLoader;
  delete _fontLoader;
  delete _stagingFontLoader;

  _destroyFrameResources();
  vkDestroyCommandPool(_base.device, _generalCommandPool, nullptr);
  _swapchain.destroyResources(_base.device);
  vkDestroySwapchainKHR(_base.device, _swapchain.swapChain, nullptr);
  vkDestroyDevice(_base.device, nullptr);
  vkDestroySurfaceKHR(_instance, _surface, nullptr);
#ifndef NDEBUG
  part::destroy::DebugMessenger(_instance, _debugMessenger, nullptr);
#endif
  vkDestroyInstance(_instance, nullptr);
}

void Render::_initFrameResources()
{
  if (_swapchain.swapChain != VK_NULL_HANDLE)
    _swapchain.destroyResources(_base.device);
  auto images = part::create::Swapchain(
      _base.device, _base.physicalDevice, _surface, &_swapchain.swapChain,
      &_swapchain.format, &_swapchain.swapchainExtent, _window, vsync);
  size_t frameCount = images.size();

  _swapchain.frameData.resize(frameCount);

  for (size_t i = 0; i < frameCount; i++) {
    _swapchain.frameData[i].SetPerFramData(
        _base.device, images[i], _swapchain.format.format,
        _base.queue.graphicsPresentFamilyIndex);
  }

  if (_forceTargetResolution)
      _swapchain.offscreenExtent = {(uint32_t)_targetResolution.x,
				    (uint32_t)_targetResolution.y};
  else
    _swapchain.offscreenExtent = _swapchain.swapchainExtent;

  // create attachment resources
  //
  VkFormat depthBufferFormat = vkhelper::findSupportedFormat(
      _base.physicalDevice,
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
       VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

  if (settings::MULTISAMPLING)
    _swapchain.maxMsaaSamples = vkhelper::getMaxSupportedMsaaSamples(
        _base.device, _base.physicalDevice);
  else
    _swapchain.maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;

  VkDeviceSize totalMemory = 0;
  uint32_t memoryFlagBits = 0;
  for (size_t i = 0; i < _swapchain.frameData.size(); i++) {
    VkMemoryRequirements memReq;
    if (settings::MULTISAMPLING) {
      _swapchain.frameData[i].multisampling.format = _swapchain.format.format;
      _swapchain.frameData[i].multisampling.memoryOffset = totalMemory;
      memReq = part::create::Image(_base.device, _base.physicalDevice,
                                   &_swapchain.frameData[i].multisampling.image,
                                   VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                       VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                   _swapchain.offscreenExtent,
                                   _swapchain.frameData[i].multisampling.format,
                                   _swapchain.maxMsaaSamples);
      totalMemory += memReq.size;
      memoryFlagBits |= memReq.memoryTypeBits;
    }

    _swapchain.frameData[i].depthBuffer.format = depthBufferFormat;
    _swapchain.frameData[i].depthBuffer.memoryOffset = totalMemory;
    memReq = part::create::Image(
        _base.device, _base.physicalDevice,
        &_swapchain.frameData[i].depthBuffer.image,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, _swapchain.offscreenExtent,
        _swapchain.frameData[i].depthBuffer.format, _swapchain.maxMsaaSamples);
    totalMemory += memReq.size;
    memoryFlagBits |= memReq.memoryTypeBits;

    _swapchain.frameData[i].offscreen.format = _swapchain.format.format;
    _swapchain.frameData[i].offscreen.memoryOffset = totalMemory;
    memReq = part::create::Image(
        _base.device, _base.physicalDevice,
        &_swapchain.frameData[i].offscreen.image,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        _swapchain.offscreenExtent, _swapchain.frameData[i].offscreen.format,
        VK_SAMPLE_COUNT_1_BIT);
    totalMemory += memReq.size;
    memoryFlagBits |= memReq.memoryTypeBits;
  }

  vkhelper::createMemory(_base.device, _base.physicalDevice, totalMemory,
                         &_swapchain.attachmentMemory,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryFlagBits);

  for (size_t i = 0; i < _swapchain.frameData.size(); i++) {
    if (settings::MULTISAMPLING) {
      vkBindImageMemory(_base.device,
                        _swapchain.frameData[i].multisampling.image,
                        _swapchain.attachmentMemory,
                        _swapchain.frameData[i].multisampling.memoryOffset);
      part::create::ImageView(_base.device,
                              &_swapchain.frameData[i].multisampling.view,
                              _swapchain.frameData[i].multisampling.image,
                              _swapchain.frameData[i].multisampling.format,
                              VK_IMAGE_ASPECT_COLOR_BIT);
    }

    vkBindImageMemory(_base.device, _swapchain.frameData[i].depthBuffer.image,
                      _swapchain.attachmentMemory,
                      _swapchain.frameData[i].depthBuffer.memoryOffset);
    part::create::ImageView(
        _base.device, &_swapchain.frameData[i].depthBuffer.view,
        _swapchain.frameData[i].depthBuffer.image,
        _swapchain.frameData[i].depthBuffer.format, VK_IMAGE_ASPECT_DEPTH_BIT);

    vkBindImageMemory(_base.device, _swapchain.frameData[i].offscreen.image,
                      _swapchain.attachmentMemory,
                      _swapchain.frameData[i].offscreen.memoryOffset);
    part::create::ImageView(
        _base.device, &_swapchain.frameData[i].offscreen.view,
        _swapchain.frameData[i].offscreen.image,
        _swapchain.frameData[i].offscreen.format, VK_IMAGE_ASPECT_COLOR_BIT);
  }

  part::create::RenderPass(_base.device, &_renderPass, _swapchain, false);
  for (size_t i = 0; i < _swapchain.frameData.size(); i++) {
    std::vector<VkImageView> offscreenAttachments;
    if (settings::MULTISAMPLING)
      offscreenAttachments = {_swapchain.frameData[i].multisampling.view,
                              _swapchain.frameData[i].depthBuffer.view,
                              _swapchain.frameData[i].offscreen.view};
    else
      offscreenAttachments = {_swapchain.frameData[i].offscreen.view,
                              _swapchain.frameData[i].depthBuffer.view};
    part::create::Framebuffer(
        _base.device, _renderPass,
        &_swapchain.frameData[i].offscreenFramebuffer, offscreenAttachments,
        _swapchain.offscreenExtent.width, _swapchain.offscreenExtent.height);
  }
  part::create::RenderPass(_base.device, &_finalRenderPass, _swapchain, true);
  for (size_t i = 0; i < _swapchain.frameData.size(); i++)
    part::create::Framebuffer(
        _base.device, _finalRenderPass, &_swapchain.frameData[i].framebuffer,
        {_swapchain.frameData[i].view}, _swapchain.swapchainExtent.width,
        _swapchain.swapchainExtent.height);

  /// set shader  descripor sets

  /// vertex descripor sets

  _VP3D.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &_VP3Dds);
  part::create::DescriptorSetLayout(_base.device, &_VP3Dds, {&_VP3D.binding},
                                    VK_SHADER_STAGE_VERTEX_BIT);

  _VP2D.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &_VP2Dds);
  part::create::DescriptorSetLayout(_base.device, &_VP2Dds, {&_VP2D.binding},
                                    VK_SHADER_STAGE_VERTEX_BIT);

  _perInstance.setSingleStructArrayBufferProps(
      frameCount, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &_perInstance3Dds,
      MAX_3D_INSTANCE);
  part::create::DescriptorSetLayout(_base.device, &_perInstance3Dds,
                                    {&_perInstance.binding},
                                    VK_SHADER_STAGE_VERTEX_BIT);

  _bones.setDynamicBufferProps(frameCount,
                               VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                               &_bonesds, 1, MAX_ANIMATIONS_PER_FRAME);
  part::create::DescriptorSetLayout(_base.device, &_bonesds, {&_bones.binding},
                                    VK_SHADER_STAGE_VERTEX_BIT);

  _per2Dvert.setSingleStructArrayBufferProps(frameCount,
                                             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                             &_per2DVertds, MAX_2D_INSTANCE);
  part::create::DescriptorSetLayout(_base.device, &_per2DVertds,
                                    {&_per2Dvert.binding},
                                    VK_SHADER_STAGE_VERTEX_BIT);

  _offscreenTransform.setBufferProps(frameCount,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &_offscreenTransformds);
  part::create::DescriptorSetLayout(_base.device, &_offscreenTransformds, {&_offscreenTransform.binding}, VK_SHADER_STAGE_VERTEX_BIT);

  // fragment descriptor sets

  _lighting.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                           &_lightingds);
  part::create::DescriptorSetLayout(_base.device, &_lightingds,
                                    {&_lighting.binding},
                                    VK_SHADER_STAGE_FRAGMENT_BIT);

  _textureSampler.setSamplerBufferProps(frameCount, VK_DESCRIPTOR_TYPE_SAMPLER,
                                        &_texturesds, 1,
                                        _textureLoader->getSamplerP());
  _textureViews.setImageViewBufferProps(
      frameCount, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &_texturesds,
      Resource::MAX_TEXTURES_SUPPORTED, _textureLoader->getImageViewsP());
  part::create::DescriptorSetLayout(
      _base.device, &_texturesds,
      {&_textureSampler.binding, &_textureViews.binding},
      VK_SHADER_STAGE_FRAGMENT_BIT);

  _per2Dfrag.setSingleStructArrayBufferProps(frameCount,
                                             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                             &_per2Dfragds, MAX_2D_INSTANCE);
  part::create::DescriptorSetLayout(_base.device, &_per2Dfragds,
                                    {&_per2Dfrag.binding},
                                    VK_SHADER_STAGE_FRAGMENT_BIT);

  part::create::DescriptorSetLayout(_base.device, &_emptyds,
				    {},
                                    VK_SHADER_STAGE_VERTEX_BIT);

  
  _offscreenTextureSampler = vkhelper::createTextureSampler(_base.device, _base.physicalDevice, 1.0f, false, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

  _offscreenSampler.setSamplerBufferProps(
      frameCount, VK_DESCRIPTOR_TYPE_SAMPLER, &_offscreends, 1,
      &_offscreenTextureSampler);
  std::vector<VkImageView> offscreenViews;
  for (size_t i = 0; i < _swapchain.frameData.size(); i++)
    offscreenViews.push_back(_swapchain.frameData[i].offscreen.view);

  _offscreenView.setPerFrameImageViewBufferProps(
      frameCount, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &_offscreends,
      offscreenViews.data());
  part::create::DescriptorSetLayout(
      _base.device, &_offscreends,
      {&_offscreenSampler.binding, &_offscreenView.binding},
      VK_SHADER_STAGE_FRAGMENT_BIT);

  // create descripor pool

  part::create::DescriptorPoolAndSet(
      _base.device, &_descPool,
      {&_VP3Dds, &_VP2Dds, &_perInstance3Dds, &_bonesds, &_per2DVertds, &_offscreenTransformds, &_lightingds, &_texturesds, &_per2Dfragds, &_offscreends},
      static_cast<uint32_t>(frameCount));

  // create memory mapped buffer for all descriptor set bindings
  part::create::PrepareShaderBufferSets(
      _base,
      {&_VP3D.binding, &_VP2D.binding, &_perInstance.binding, &_bones.binding,
       &_per2Dvert.binding, &_lighting.binding,
       &_offscreenTransform.binding,
       &_textureSampler.binding, &_textureViews.binding,
       &_per2Dfrag.binding,
       &_offscreenSampler.binding, &_offscreenView.binding},
      &_shaderBuffer, &_shaderMemory);

  // create pipeline for each shader set -> 3D, animated 3D, 2D, and final
  part::create::GraphicsPipeline(
      _base.device, &_pipeline3D, _swapchain, _renderPass,
      {&_VP3Dds, &_perInstance3Dds, &_emptyds, &_texturesds, &_lightingds},
      {{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(fragPushConstants)}},
      "shaders/vulkan/3D-lighting.vert.spv", "shaders/vulkan/blinnphong.frag.spv", true,
      settings::MULTISAMPLING, true, _swapchain.offscreenExtent,
      VK_CULL_MODE_BACK_BIT, Vertex3D::attributeDescriptions(),
      Vertex3D::bindingDescriptions());

  part::create::GraphicsPipeline(
      _base.device, &_pipelineAnim3D, _swapchain, _renderPass,
      {&_VP3Dds, &_perInstance3Dds, &_bonesds, &_texturesds, &_lightingds},
      {{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(fragPushConstants)}},
      "shaders/vulkan/3D-lighting-anim.vert.spv", "shaders/vulkan/blinnphong.frag.spv",
      true, settings::MULTISAMPLING, true, _swapchain.offscreenExtent,
      VK_CULL_MODE_BACK_BIT, VertexAnim3D::attributeDescriptions(),
      VertexAnim3D::bindingDescriptions());

  part::create::GraphicsPipeline(
      _base.device, &_pipeline2D, _swapchain, _renderPass,
      {&_VP2Dds, &_per2DVertds, &_texturesds, &_per2Dfragds}, {},
      "shaders/vulkan/flat.vert.spv", "shaders/vulkan/flat.frag.spv", true,
      settings::MULTISAMPLING, true, _swapchain.offscreenExtent,
      VK_CULL_MODE_BACK_BIT, Vertex2D::attributeDescriptions(),
      Vertex2D::bindingDescriptions());

  part::create::GraphicsPipeline(
      _base.device, &_pipelineFinal, _swapchain, _finalRenderPass,
      {&_offscreenTransformds, &_offscreends}, {}, "shaders/vulkan/final.vert.spv", "shaders/vulkan/final.frag.spv",
      false, false, false, _swapchain.swapchainExtent, VK_CULL_MODE_NONE, {},
      {});


  float ratio = ((float)_swapchain.offscreenExtent.width / (float)_swapchain.offscreenExtent.height) * ((float)_swapchain.swapchainExtent.height / (float)_swapchain.swapchainExtent.width);
   _offscreenTransform.data[0] = glm::scale(glm::mat4(1.0f), glm::vec3(ratio < 1.0f ? ratio: 1.0f, ratio > 1.0f ? 1.0f / ratio : 1.0f, 1.0f));
}

void Render::_destroyFrameResources()
{
  vkDestroyBuffer(_base.device, _shaderBuffer, nullptr);
  vkFreeMemory(_base.device, _shaderMemory, nullptr);

  vkDestroySampler(_base.device, _offscreenTextureSampler, nullptr);

  _VP3Dds.destroySet(_base.device);
  _VP2Dds.destroySet(_base.device);
  _perInstance3Dds.destroySet(_base.device);
  _per2DVertds.destroySet(_base.device);
  _bonesds.destroySet(_base.device);
  _offscreenTransformds.destroySet(_base.device);
  _lightingds.destroySet(_base.device);
  _texturesds.destroySet(_base.device);
  _per2Dfragds.destroySet(_base.device);
  _offscreends.destroySet(_base.device);
  _emptyds.destroySet(_base.device);

  vkDestroyDescriptorPool(_base.device, _descPool, nullptr);

  for (size_t i = 0; i < _swapchain.frameData.size(); i++) {
    vkDestroyFramebuffer(_base.device, _swapchain.frameData[i].framebuffer,
                         nullptr);
    vkDestroyFramebuffer(_base.device,
                         _swapchain.frameData[i].offscreenFramebuffer, nullptr);
  }
  _pipeline3D.destroy(_base.device);
  _pipelineAnim3D.destroy(_base.device);
  _pipeline2D.destroy(_base.device);
  _pipelineFinal.destroy(_base.device);
  vkDestroyRenderPass(_base.device, _renderPass, nullptr);
  vkDestroyRenderPass(_base.device, _finalRenderPass, nullptr);
}

Resource::Texture Render::LoadTexture(std::string filepath) {
  return _stagingTextureLoader->LoadTexture(filepath);
}

Resource::Font Render::LoadFont(std::string filepath) {
  try {
    return _stagingFontLoader->LoadFont(filepath, _stagingTextureLoader);
  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
    return Resource::Font();
  }
}

Resource::Model Render::LoadAnimatedModel(
    std::string filepath,
    std::vector<Resource::ModelAnimation> *pGetAnimations)
{
  return _stagingModelLoader->loadModel(filepath, _stagingTextureLoader, pGetAnimations);
}

Resource::Model Render::LoadModel(std::string filepath)
{
  return _stagingModelLoader->loadModel(filepath, _stagingTextureLoader);
}

void Render::LoadResourcesToGPU() {
  _stagingTextureLoader->endLoading();
  _stagingModelLoader->endLoading(_transferCommandBuffer);
}

void Render::UseLoadedResources()
{
  vkDeviceWaitIdle(_base.device);
  if(_textureLoader != nullptr)
      _destroyFrameResources();
  delete _textureLoader;
  _textureLoader = _stagingTextureLoader;
  delete _modelLoader;
  _modelLoader = _stagingModelLoader;
  delete _fontLoader;
  _fontLoader = _stagingFontLoader;
  _initStagingResourceManagers();
  _initFrameResources();
}

void Render::_resize()
{
  vkDeviceWaitIdle(_base.device);

  _destroyFrameResources();
  _initFrameResources();

  vkDeviceWaitIdle(_base.device);
  _updateViewProjectionMatrix();
}

void Render::_startDraw()
{
  if (_textureLoader == nullptr)
    throw std::runtime_error(
        "resource loading must be finished before drawing to screen!");
  _begunDraw = true;

  if (_swapchain.imageAquireSem.empty()) {
    VkSemaphoreCreateInfo semaphoreInfo{
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    if (vkCreateSemaphore(_base.device, &semaphoreInfo, nullptr,
                          &_imgAquireSem) != VK_SUCCESS)
      throw std::runtime_error("failed to create image available semaphore");
  } else {
    _imgAquireSem = _swapchain.imageAquireSem.back();
    _swapchain.imageAquireSem.pop_back();
  }
  if (vkAcquireNextImageKHR(_base.device, _swapchain.swapChain, UINT64_MAX,
                            _imgAquireSem, VK_NULL_HANDLE,
                            &_frameI) != VK_SUCCESS) {
    _swapchain.imageAquireSem.push_back(_imgAquireSem);
    return;
  }

  if (_swapchain.frameData[_frameI].frameFinishedFen != VK_NULL_HANDLE) {
    vkWaitForFences(_base.device, 1,
                    &_swapchain.frameData[_frameI].frameFinishedFen, VK_TRUE,
                    UINT64_MAX);
    vkResetFences(_base.device, 1,
                  &_swapchain.frameData[_frameI].frameFinishedFen);
  }
  vkResetCommandPool(_base.device, _swapchain.frameData[_frameI].commandPool, 0);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  beginInfo.pInheritanceInfo = nullptr;

  if (vkBeginCommandBuffer(_swapchain.frameData[_frameI].commandBuffer,
                           &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to being recording command buffer");
  }

  // fill render pass begin struct
  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = _renderPass;
  renderPassInfo.framebuffer =
      _swapchain.frameData[_frameI].offscreenFramebuffer; // framebuffer for each swapchain image
                                       // should match size of attachments
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = _swapchain.offscreenExtent;
  // clear colour -> values for VK_ATTACHMENT_LOAD_OP_CLEAR load operation in
  // colour attachment need colour for each attachment being cleared (colour,
  // depth)
  std::array<VkClearValue, 2> clearColours{};
  clearColours[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearColours[1].depthStencil = {1.0f, 0};
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearColours.size());
  renderPassInfo.pClearValues = clearColours.data();

  vkCmdBeginRenderPass(_swapchain.frameData[_frameI].commandBuffer,
                       &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{
      0.0f,
      0.0f, // x  y
      (float)_swapchain.offscreenExtent.width,
      (float)_swapchain.offscreenExtent.height, // width  height
      0.0f,
      1.0f // min/max depth
  };
  VkRect2D scissor{VkOffset2D{0, 0}, _swapchain.offscreenExtent};
  vkCmdSetViewport(_swapchain.frameData[_frameI].commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(_swapchain.frameData[_frameI].commandBuffer, 0, 1, &scissor);

  _modelLoader->bindBuffers(_swapchain.frameData[_frameI].commandBuffer);
  _bones.currentDynamicOffsetIndex = 0;
}

void Render::Begin3DDraw()
{
  if (!_begunDraw)
    _startDraw();
  if (_modelRuns > 0)
    _drawBatch();
  if (_instance2Druns > 0)
    _drawBatch();
  _renderState = RenderState::Draw3D;

  _VP3D.storeData(_frameI);
  _lighting.data[0].direction = _lightDirection;
  _lighting.storeData(_frameI);

  _pipeline3D.begin(_swapchain.frameData[_frameI].commandBuffer, _frameI);
}

void Render::DrawModel(Resource::Model model, glm::mat4 modelMatrix, glm::mat4 normalMat)
{
  if (_current3DInstanceIndex >= MAX_3D_INSTANCE) {
    std::cout << "WARNING: ran out of 3D instances!\n";
    return;
  }

  if (_currentModel.ID != model.ID && _modelRuns != 0)
    _drawBatch();

  _currentModel = model;
  _perInstance.data[_current3DInstanceIndex + _modelRuns].model = modelMatrix;
  _perInstance.data[_current3DInstanceIndex + _modelRuns].normalMat = normalMat;
  _modelRuns++;

  if (_current3DInstanceIndex + _modelRuns == MAX_3D_INSTANCE)
    _drawBatch();
}

void Render::BeginAnim3DDraw()
{
  if (!_begunDraw)
    _startDraw();
  if (_modelRuns > 0)
    _drawBatch();
  if (_instance2Druns > 0)
    _drawBatch();
  _renderState = RenderState::DrawAnim3D;

  _VP3D.storeData(_frameI);
  _lighting.data[0].direction = _lightDirection;
  _lighting.storeData(_frameI);
  _pipelineAnim3D.begin(_swapchain.frameData[_frameI].commandBuffer, _frameI);
}

void Render::DrawAnimModel(Resource::Model model, glm::mat4 modelMatrix, glm::mat4 normalMat, Resource::ModelAnimation *animation)
{
   if (_current3DInstanceIndex >= MAX_3D_INSTANCE) {
     std::cout << "WARNING: Ran out of 3D Anim Instance models!\n";
     return;
  }

  if (_currentModel.ID != model.ID && _modelRuns != 0)
    _drawBatch();

  _currentModel = model;
  _perInstance.data[_current3DInstanceIndex + _modelRuns].model = modelMatrix;
  _perInstance.data[_current3DInstanceIndex + _modelRuns].normalMat = normalMat;
  _modelRuns++;

  auto bones = animation->getCurrentBones();
  for(int i = 0; i < bones->size() && i < 50; i++)
  {
    _bones.data[0].mat[i] = bones->at(i);
  }
  if(_bones.currentDynamicOffsetIndex >= MAX_ANIMATIONS_PER_FRAME)
  {
    std::cout << "warning, too many animation calls!\n";
    return;
  }
  _bones.storeData(_frameI);
  uint32_t offset = static_cast<uint32_t>((_bones.currentDynamicOffsetIndex-1) * _bones.binding.bufferSize * _bones.binding.setCount);
  _pipelineAnim3D.bindDynamicDS(_swapchain.frameData[_frameI].commandBuffer, &_bonesds, _frameI,  offset);
   _drawBatch();
}

void Render::Begin2DDraw()
{
  if (!_begunDraw)
    _startDraw();
  if (_modelRuns > 0)
    _drawBatch();
  if (_instance2Druns > 0)
    _drawBatch();
  _renderState = RenderState::Draw2D;

  float correction;
  float deviceRatio = (float)_swapchain.offscreenExtent.width /
                  (float)_swapchain.offscreenExtent.height;
  float virtualRatio = _targetResolution.x / _targetResolution.y;
  float xCorrection = _swapchain.offscreenExtent.width / _targetResolution.x;
  float yCorrection = _swapchain.offscreenExtent.height / _targetResolution.y;

  if (virtualRatio < deviceRatio) {
    correction = yCorrection;
  } else {
    correction = xCorrection;
  }
  _VP2D.data[0].proj = glm::ortho(
      0.0f, (float)_swapchain.offscreenExtent.width*_scale2D / correction, 0.0f,
      (float)_swapchain.offscreenExtent.height*_scale2D / correction, -10.0f, 10.0f);
  _VP2D.data[0].view = glm::mat4(1.0f);

  _VP2D.storeData(_frameI);

  _pipeline2D.begin(_swapchain.frameData[_frameI].commandBuffer, _frameI);
}

void Render::DrawQuad(Resource::Texture texture, glm::mat4 modelMatrix, glm::vec4 colour, glm::vec4 texOffset)
{
  if (_current2DInstanceIndex >= MAX_2D_INSTANCE)
  {
    std::cout << "WARNING: ran out of 2D instance models!\n";
    return;
  }

  _per2Dvert.data[_current2DInstanceIndex + _instance2Druns] = modelMatrix;
  _per2Dfrag.data[_current2DInstanceIndex + _instance2Druns].colour = colour;
  _per2Dfrag.data[_current2DInstanceIndex + _instance2Druns].texOffset = texOffset;
  _per2Dfrag.data[_current2DInstanceIndex + _instance2Druns].texID = (uint32_t)texture.ID;
  _instance2Druns++;

  if (_current2DInstanceIndex + _instance2Druns == MAX_2D_INSTANCE)
    _drawBatch();
}

void Render::DrawQuad(Resource::Texture texture, glm::mat4 modelMatrix, glm::vec4 colour)
{
  DrawQuad(texture, modelMatrix, colour, glm::vec4(0, 0, 1, 1));
}

void Render::DrawQuad(Resource::Texture texture, glm::mat4 modelMatrix) {
  DrawQuad(texture, modelMatrix, glm::vec4(1), glm::vec4(0, 0, 1, 1));
}

void Render::DrawString(Resource::Font font, std::string text, glm::vec2 position, float size, float depth, glm::vec4 colour, float rotate)
{
  auto draws = _fontLoader->DrawString(font, text, position, size, depth, colour, rotate);
  for (const auto &draw : draws)
  {
    DrawQuad(draw.tex, draw.model, draw.colour, draw.texOffset);
  }
}
void Render::DrawString(Resource::Font font, std::string text, glm::vec2 position, float size, float depth, glm::vec4 colour)
{
  DrawString(font, text, position, size, depth, colour, 0.0);
}

float Render::MeasureString(Resource::Font font, std::string text, float size)
{
  return _fontLoader->MeasureString(font, text, size);
}

void Render::_drawBatch()
{
  switch(_renderState)
  {
       case RenderState::DrawAnim3D:
       case RenderState::Draw3D:
         _modelLoader->drawModel(_swapchain.frameData[_frameI].commandBuffer,
                            _pipeline3D.layout, _currentModel, _modelRuns,
                            _current3DInstanceIndex);
      _current3DInstanceIndex += _modelRuns;
      _modelRuns = 0;
      break;
    case RenderState::Draw2D:
      _modelLoader->drawQuad(_swapchain.frameData[_frameI].commandBuffer,
                           _pipeline3D.layout, 0, _instance2Druns,
                           _current2DInstanceIndex, _currentColour,
                           _currentTexOffset);
      _current2DInstanceIndex += _instance2Druns;
      _instance2Druns = 0;
      break;
  }
}

void Render::EndDraw(std::atomic<bool> &submit) {
  if (!_begunDraw)
    throw std::runtime_error("start draw before ending it");

  _begunDraw = false;

  switch(_renderState)
  {
    case RenderState::Draw3D:
    case RenderState::DrawAnim3D:
      if (_modelRuns != 0 && _current3DInstanceIndex < MAX_3D_INSTANCE)
        _drawBatch();
      break;
    case RenderState::Draw2D:
      if (_instance2Druns != 0 && _current2DInstanceIndex < MAX_2D_INSTANCE)
        _drawBatch();
      break;
  }

  for (size_t i = 0; i < _current3DInstanceIndex; i++)
    _perInstance.storeData(_frameI, 0, i);
  _current3DInstanceIndex = 0;


  for (size_t i = 0; i < _current2DInstanceIndex; i++)
  {
    _per2Dvert.storeData(_frameI, 0, i);
    _per2Dfrag.storeData(_frameI, 0, i);
  }
  _current2DInstanceIndex = 0;

  // end last offscreen render pass
  vkCmdEndRenderPass(_swapchain.frameData[_frameI].commandBuffer);

  // final, onscreen render pass
  
  // fill render pass begin struct
  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = _finalRenderPass;
  renderPassInfo.framebuffer =
      _swapchain.frameData[_frameI]
          .framebuffer; // framebuffer for each swapchain image
                        // should match size of attachments
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = _swapchain.swapchainExtent;

  std::array<VkClearValue, 1> clearColours{};
  clearColours[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearColours.size());
  renderPassInfo.pClearValues = clearColours.data();

  _offscreenTransform.storeData(_frameI);

  vkCmdBeginRenderPass(_swapchain.frameData[_frameI].commandBuffer,
                       &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{
      0.0f,
      0.0f, // x  y
      (float)_swapchain.swapchainExtent.width,
      (float)_swapchain.swapchainExtent.height, // width  height
      0.0f,
      1.0f // min/max depth
  };
  VkRect2D scissor{VkOffset2D{0, 0}, _swapchain.swapchainExtent};
  vkCmdSetViewport(_swapchain.frameData[_frameI].commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(_swapchain.frameData[_frameI].commandBuffer, 0, 1, &scissor);

  _pipelineFinal.begin(_swapchain.frameData[_frameI].commandBuffer, _frameI);

  vkCmdDraw(_swapchain.frameData[_frameI].commandBuffer, 3, 1, 0, 0);

  vkCmdEndRenderPass(_swapchain.frameData[_frameI].commandBuffer);

  // final render pass end

  if (vkEndCommandBuffer(_swapchain.frameData[_frameI].commandBuffer) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }

  std::array<VkSemaphore, 1> submitWaitSemaphores = {_imgAquireSem};
  std::array<VkPipelineStageFlags, 1> waitStages = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  std::array<VkSemaphore, 1> submitSignalSemaphores = {
      _swapchain.frameData[_frameI].presentReadySem};

  // submit draw command
  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr};
  submitInfo.waitSemaphoreCount = static_cast<uint32_t>(submitWaitSemaphores.size());
  submitInfo.pWaitSemaphores = submitWaitSemaphores.data();
  submitInfo.pWaitDstStageMask = waitStages.data();
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &_swapchain.frameData[_frameI].commandBuffer;
  submitInfo.signalSemaphoreCount = static_cast<uint32_t>(submitSignalSemaphores.size());
  submitInfo.pSignalSemaphores = submitSignalSemaphores.data();
  if (vkQueueSubmit(_base.queue.graphicsPresentQueue, 1, &submitInfo,
                    _swapchain.frameData[_frameI].frameFinishedFen) != VK_SUCCESS)
    throw std::runtime_error("failed to submit draw command buffer");

  // submit present command
  VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr};
  presentInfo.waitSemaphoreCount = static_cast<uint32_t>(submitSignalSemaphores.size());
  presentInfo.pWaitSemaphores = submitSignalSemaphores.data();
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &_swapchain.swapChain;
  presentInfo.pImageIndices = &_frameI;
  presentInfo.pResults = nullptr;

  VkResult result =
      vkQueuePresentKHR(_base.queue.graphicsPresentQueue, &presentInfo);
vkDeviceWaitIdle(_base.device);
  if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR ||
      _framebufferResized) {
    _framebufferResized = false;
    _resize();
  } else if (result != VK_SUCCESS)
    throw std::runtime_error("failed to present swapchain image to queue");

  _swapchain.imageAquireSem.push_back(_imgAquireSem);

  submit = true;
}

void Render::_updateViewProjectionMatrix() {
  _VP3D.data[0].proj =
      glm::perspective(glm::radians(_projectionFov),
                       ((float)_swapchain.offscreenExtent.width) /
                           ((float)_swapchain.offscreenExtent.height),
                       0.1f, 500.0f);
  _VP3D.data[0].proj[1][1] *=
      -1; // opengl has inversed y axis, so need to correct
}

//recreates frame resources, so any state change for rendering will be updated on next draw if this is called
void Render::FramebufferResize() { _framebufferResized = true; }

void Render::set3DViewMatrixAndFov(glm::mat4 view, float fov, glm::vec4 camPos) {
  _VP3D.data[0].view = view;
  _projectionFov = fov;
  _lighting.data[0].camPos = camPos;
  _updateViewProjectionMatrix();
}

void Render::set2DViewMatrixAndScale(glm::mat4 view, float scale)
{
  _VP2D.data[0].view = view;
  _scale2D = scale;
}

void Render::setLightDirection(glm::vec4 lightDir)
{
  _lightDirection = lightDir;
}

void Render::setForceTargetRes(bool force) {
    if(_forceTargetResolution != force) {
	_forceTargetResolution = force;
	FramebufferResize();
    }
}
bool Render::isTargetResForced() { return _forceTargetResolution; }
void Render::setTargetResolution(glm::vec2 resolution) {
    _targetResolution = resolution;
    _forceTargetResolution = true;
    FramebufferResize();
}
glm::vec2 Render::getTargetResolution() {
    return _targetResolution;
}
void Render::setVsync(bool vsync) {
    this->vsync = vsync;
    FramebufferResize();
}

}//namespace
