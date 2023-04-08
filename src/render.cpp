#include "render.h"

#include "descriptor_structs.h"
#include "parts/render_style.h"
#include "resources/model_render.h"
#include "resources/font_loader.h"
#include "resources/texture_loader.h"
#include "parts/core.h"
#include "parts/swapchain.h"
#include "parts/descriptors.h"
#include "parts/command.h"
#include "pipeline.h"
#include "vkhelper.h"
#include "logger.h"

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


Render::Render(GLFWwindow *window, glm::vec2 target)
{
    checkVolk();
    EnabledFeatures features;
    features.sampleRateShading = renderConf.sample_shading;
    manager = new VulkanManager(window, features);
    _targetResolution = target;
    swapchain = new Swapchain(manager->deviceState,  manager->windowSurface);
    _initStagingResourceManagers();
}

void Render::_initStagingResourceManagers() {
  _stagingModelLoader = new Resource::ModelRender(manager->deviceState, manager->generalCommandPool);
  _stagingTextureLoader = new Resource::TextureLoader(manager->deviceState,
						      manager->generalCommandPool,
						      renderConf);
  _stagingFontLoader = new Resource::FontLoader();
  _stagingTextureLoader->LoadTexture("textures/error.png");
}

Render::~Render()
{
  vkQueueWaitIdle(manager->deviceState.queue.graphicsPresentQueue);

  delete _textureLoader;
  delete _stagingTextureLoader;
  delete _modelLoader;
  delete _stagingModelLoader;
  delete _fontLoader;
  delete _stagingFontLoader;
  _destroyFrameResources();
  delete swapchain;
  delete manager;
}

bool swapchainRecreationRequired(VkResult result) {
    return result == VK_SUBOPTIMAL_KHR ||
	result == VK_ERROR_OUT_OF_DATE_KHR;
}

  void Render::_initFrameResources()
  {
      
      LOG("Creating Swapchain");
	    
      int winWidth, winHeight;
      glfwGetFramebufferSize(manager->window, &winWidth, &winHeight);
      VkExtent2D offscreenBufferExtent = {(uint32_t)winWidth, (uint32_t)winHeight};
      if (renderConf.force_target_resolution)
	  offscreenBufferExtent = {(uint32_t)_targetResolution.x,
	      (uint32_t)_targetResolution.y};

      VkExtent2D swapchainExtent = {(uint32_t)winWidth, (uint32_t)winHeight};
      VkResult result = swapchain->InitFrameResources(swapchainExtent,
						      offscreenBufferExtent,
						      renderConf);

      if(result !=  VK_SUCCESS) {
	  //TODO check if out of date and try recreate?
	  throw std::runtime_error(
		  GET_ERR_STRING("failed to create swapchain resources", result));
      }
      
      size_t frameCount = swapchain->frameCount();
      

      LOG("Creating Descriptor Sets");
      
      /// set shader  descripor sets

      descriptor::Descriptor viewProjectionBinding("view projection struct",
						   descriptor::DescriptorType::UniformBuffer,
						   sizeof(DS::ShaderStructs::viewProjection), 1);
      
      /// vertex descripor sets
      descriptor::Set VP3D_Set("VP3D", descriptor::ShaderStage::Vertex);
      VP3D_Set.AddDescriptor(viewProjectionBinding);
      VP3D = new DescSet(VP3D_Set, frameCount, manager->deviceState.device);

      descriptor::Set VP2D_Set("VP2D", descriptor::ShaderStage::Vertex);
      VP2D_Set.AddDescriptor(viewProjectionBinding);
      VP2D = new DescSet(VP2D_Set, frameCount, manager->deviceState.device);

      
      timeData.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &timeds);
      part::create::DescriptorSetLayout(manager->deviceState.device, &timeds, {&timeData.binding},
					VK_SHADER_STAGE_VERTEX_BIT);

      descriptor::Set PerFrame3D_Set("Per Frame 3D", descriptor::ShaderStage::Vertex);
      PerFrame3D_Set.AddSingleArrayStructDescriptor("3D Instance Array", descriptor::DescriptorType::StorageBuffer,
				   sizeof(DS::ShaderStructs::PerFrame3D), MAX_3D_INSTANCE);
      perFrame3D = new DescSet(PerFrame3D_Set, frameCount, manager->deviceState.device);
      
      
      _bones.setDynamicBufferProps(frameCount,
				   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
				   &_bonesds, 1, MAX_ANIMATIONS_PER_FRAME);
      part::create::DescriptorSetLayout(manager->deviceState.device, &_bonesds, {&_bones.binding},
					VK_SHADER_STAGE_VERTEX_BIT);

      _per2Dvert.setSingleStructArrayBufferProps(frameCount,
						 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						 &_per2DVertds, MAX_2D_INSTANCE);
      part::create::DescriptorSetLayout(manager->deviceState.device, &_per2DVertds,
					{&_per2Dvert.binding},
					VK_SHADER_STAGE_VERTEX_BIT);

      _offscreenTransform.setBufferProps(frameCount,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &_offscreenTransformds);
      part::create::DescriptorSetLayout(manager->deviceState.device, &_offscreenTransformds, {&_offscreenTransform.binding}, VK_SHADER_STAGE_VERTEX_BIT);

      // fragment descriptor sets

      _lighting.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			       &_lightingds);
      part::create::DescriptorSetLayout(manager->deviceState.device, &_lightingds,
					{&_lighting.binding},
					VK_SHADER_STAGE_FRAGMENT_BIT);

      _textureSampler.setSamplerBufferProps(frameCount, VK_DESCRIPTOR_TYPE_SAMPLER,
					    &_texturesds, 1,
					    _textureLoader->getSamplerP());
      _textureViews.setImageViewBufferProps(
	      frameCount, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &_texturesds,
	      Resource::MAX_TEXTURES_SUPPORTED, _textureLoader->getImageViewsP());
      part::create::DescriptorSetLayout(
	      manager->deviceState.device, &_texturesds,
	      {&_textureSampler.binding, &_textureViews.binding},
	      VK_SHADER_STAGE_FRAGMENT_BIT);

      _per2Dfrag.setSingleStructArrayBufferProps(frameCount,
						 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						 &_per2Dfragds, MAX_2D_INSTANCE);
      part::create::DescriptorSetLayout(manager->deviceState.device, &_per2Dfragds,
					{&_per2Dfrag.binding},
					VK_SHADER_STAGE_FRAGMENT_BIT);

      part::create::DescriptorSetLayout(manager->deviceState.device, &_emptyds,
					{},
					VK_SHADER_STAGE_VERTEX_BIT);

  
      _offscreenTextureSampler = vkhelper::createTextureSampler(manager->deviceState.device, manager->deviceState.physicalDevice, 1.0f, false, renderConf.texture_filter_nearest, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
      _offscreenSampler.setSamplerBufferProps(
	      frameCount, VK_DESCRIPTOR_TYPE_SAMPLER, &_offscreends, 1,
	      &_offscreenTextureSampler);
      
      std::vector<VkImageView> offscreenViews = swapchain->getOffscreenViews();
      _offscreenView.setPerFrameImageViewBufferProps(
	      frameCount, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &_offscreends,
	      offscreenViews.data());
      part::create::DescriptorSetLayout(
	      manager->deviceState.device, &_offscreends,
	      {&_offscreenSampler.binding, &_offscreenView.binding},
	      VK_SHADER_STAGE_FRAGMENT_BIT);

      
      LOG("Creating Descriptor pool and memory for set bindings");

      // create descripor pool

      part::create::DescriptorPoolAndSet(
	      manager->deviceState.device, &_descPool,
	      {&VP3D->set, &VP2D->set, &perFrame3D->set, &_bonesds, &timeds,
	       &_per2DVertds, &_offscreenTransformds, &_lightingds,
	       &_texturesds, &_per2Dfragds, &_offscreends},
	      static_cast<uint32_t>(frameCount));

		  
      // create memory mapped buffer for all descriptor set bindings
      part::create::PrepareShaderBufferSets(
	      manager->deviceState,
	      {&VP3D->bindings[0], &timeData.binding, &VP2D->bindings[0], &perFrame3D->bindings[0], &_bones.binding,
	       &_per2Dvert.binding, &_lighting.binding,
	       &_offscreenTransform.binding,
	       &_textureSampler.binding, &_textureViews.binding,
	       &_per2Dfrag.binding,
	       &_offscreenSampler.binding, &_offscreenView.binding},
	      &_shaderBuffer, &_shaderMemory);

      LOG("Creating Graphics Pipelines");

      // create pipeline for each shader set -> 3D, animated 3D, 2D, and final
      part::create::GraphicsPipeline(
	      manager->deviceState.device, &_pipeline3D,
	      swapchain->getMaxMsaaSamples(), swapchain->offscreenRenderPass,
	      {&VP3D->set, &perFrame3D->set, &timeds, &_texturesds, &_lightingds},
	      {{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(fragPushConstants)}},
	      "shaders/vulkan/3D-lighting.vert.spv", "shaders/vulkan/blinnphong.frag.spv", true,
	      renderConf.multisampling, true, manager->deviceState.features.sampleRateShading, swapchain->offscreenExtent,
	      VK_CULL_MODE_BACK_BIT, Vertex3D::attributeDescriptions(),
	      Vertex3D::bindingDescriptions());

	    
      part::create::GraphicsPipeline(
	      manager->deviceState.device, &_pipelineAnim3D,
	      swapchain->getMaxMsaaSamples(), swapchain->offscreenRenderPass,
	      {&VP3D->set, &perFrame3D->set, &_bonesds, &_texturesds, &_lightingds},
	      {{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(fragPushConstants)}},
	      "shaders/vulkan/3D-lighting-anim.vert.spv", "shaders/vulkan/blinnphong.frag.spv",
	      true, renderConf.multisampling, true, manager->deviceState.features.sampleRateShading, swapchain->offscreenExtent,
	      VK_CULL_MODE_BACK_BIT, VertexAnim3D::attributeDescriptions(),
	      VertexAnim3D::bindingDescriptions());

      part::create::GraphicsPipeline(
	      manager->deviceState.device, &_pipeline2D, swapchain->getMaxMsaaSamples(),
	      swapchain->offscreenRenderPass,
	      {&VP2D->set, &_per2DVertds, &_texturesds, &_per2Dfragds}, {},
	      "shaders/vulkan/flat.vert.spv", "shaders/vulkan/flat.frag.spv", true,
	      renderConf.multisampling, true, manager->deviceState.features.sampleRateShading, swapchain->offscreenExtent,
	      VK_CULL_MODE_BACK_BIT, Vertex2D::attributeDescriptions(),
	      Vertex2D::bindingDescriptions());

      part::create::GraphicsPipeline(
	      manager->deviceState.device, &_pipelineFinal, swapchain->getMaxMsaaSamples(),
	      swapchain->finalRenderPass,
	      {&_offscreenTransformds, &_offscreends}, {}, "shaders/vulkan/final.vert.spv", "shaders/vulkan/final.frag.spv",
	      false, false, false, manager->deviceState.features.sampleRateShading,
	      swapchain->swapchainExtent, VK_CULL_MODE_NONE, {},
	      {});


      float ratio = ((float)swapchain->offscreenExtent.width /
		     (float)swapchain->offscreenExtent.height) *
	  ((float)swapchain->swapchainExtent.height /
	   (float)swapchain->swapchainExtent.width);
      _offscreenTransform.data[0] =
	glm::scale(glm::mat4(1.0f),
		   glm::vec3(ratio < 1.0f ? ratio: 1.0f,
			     ratio > 1.0f ? 1.0f / ratio : 1.0f,
			     1.0f));

      timeData.data[0].time = 0;
  }

void Render::_destroyFrameResources()
{
  vkDestroyBuffer(manager->deviceState.device, _shaderBuffer, nullptr);
  vkFreeMemory(manager->deviceState.device, _shaderMemory, nullptr);

  vkDestroySampler(manager->deviceState.device, _offscreenTextureSampler, nullptr);

  delete VP3D;
  VP3D = nullptr;
  delete VP2D;
  VP2D = nullptr;
  timeds.destroySet(manager->deviceState.device);
  delete perFrame3D;
  perFrame3D = nullptr;
  _per2DVertds.destroySet(manager->deviceState.device);
  _bonesds.destroySet(manager->deviceState.device);
  _offscreenTransformds.destroySet(manager->deviceState.device);
  _lightingds.destroySet(manager->deviceState.device);
  _texturesds.destroySet(manager->deviceState.device);
  _per2Dfragds.destroySet(manager->deviceState.device);
  _offscreends.destroySet(manager->deviceState.device);
  _emptyds.destroySet(manager->deviceState.device);

  vkDestroyDescriptorPool(manager->deviceState.device, _descPool, nullptr);

  _pipeline3D.destroy(manager->deviceState.device);
  _pipelineAnim3D.destroy(manager->deviceState.device);
  _pipeline2D.destroy(manager->deviceState.device);
  _pipelineFinal.destroy(manager->deviceState.device);

  swapchain->DestroyFrameResources();
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
  _stagingModelLoader->endLoading(manager->generalCommandBuffer);
}

void Render::UseLoadedResources()
{
  vkDeviceWaitIdle(manager->deviceState.device);
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
    _framebufferResized = false;
  vkDeviceWaitIdle(manager->deviceState.device);

  _destroyFrameResources();
  _initFrameResources();
  
  vkDeviceWaitIdle(manager->deviceState.device);
  _update3DProjectionMatrix();
}

void Render::_startDraw()
{
    bool rebuiltSwapchain = false;
 START_DRAW: // retry start draw if out of date swapchain
    VkResult result =  swapchain->beginOffscreenRenderPass(&currentCommandBuffer);
    if(result != VK_SUCCESS) {
	if(swapchainRecreationRequired(result)) {
	    _resize();
	    if(!rebuiltSwapchain) { //only try to rebuild once
		rebuiltSwapchain = true;
		goto START_DRAW;
	    }
	}
	throw std::runtime_error(
		GET_ERR_STRING("failed to begin offscreen render pass!", result));
    }
    _modelLoader->bindBuffers(currentCommandBuffer);
    _frameI  =  swapchain->getFrameIndex();
    _bones.currentDynamicOffsetIndex = 0;
    _begunDraw = true;
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

  VP3D->bindings[0].storeSetData(_frameI, &VP3DData, 0, 0, 0);
  timeData.storeData(_frameI);
  _lighting.data[0].direction = _lightDirection;
  _lighting.storeData(_frameI);

  _pipeline3D.begin(currentCommandBuffer, _frameI);
}

void Render::DrawModel(Resource::Model model, glm::mat4 modelMatrix, glm::mat4 normalMat)
{
  if (_current3DInstanceIndex >= MAX_3D_INSTANCE) {
      LOG("WARNING: ran out of 3D instances!\n");
      return;
  }

  if (_currentModel.ID != model.ID && _modelRuns != 0)
    _drawBatch();

  _currentModel = model;
  perFrame3DData[_current3DInstanceIndex + _modelRuns].model = modelMatrix;
  perFrame3DData[_current3DInstanceIndex + _modelRuns].normalMat = normalMat;
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

  VP3D->bindings[0].storeSetData(_frameI, &VP3DData, 0, 0, 0);
  _lighting.data[0].direction = _lightDirection;
  _lighting.storeData(_frameI);
  _pipelineAnim3D.begin(currentCommandBuffer, _frameI);
}

void Render::DrawAnimModel(Resource::Model model, glm::mat4 modelMatrix, glm::mat4 normalMat, Resource::ModelAnimation *animation)
{
   if (_current3DInstanceIndex >= MAX_3D_INSTANCE) {
       LOG("WARNING: Ran out of 3D Anim Instance models!\n");
       return;
  }

  if (_currentModel.ID != model.ID && _modelRuns != 0)
    _drawBatch();

  _currentModel = model;
  perFrame3DData[_current3DInstanceIndex + _modelRuns].model = modelMatrix;
  perFrame3DData[_current3DInstanceIndex + _modelRuns].normalMat = normalMat;
  _modelRuns++;

  auto bones = animation->getCurrentBones();
  for(int i = 0; i < bones->size() && i < 50; i++)
  {
    _bones.data[0].mat[i] = bones->at(i);
  }
  if(_bones.currentDynamicOffsetIndex >= MAX_ANIMATIONS_PER_FRAME)
  {
      LOG("warning, too many animation calls!\n");
      return;
  }
  _bones.storeData(_frameI);
  uint32_t offset = static_cast<uint32_t>((_bones.currentDynamicOffsetIndex-1) * _bones.binding.bufferSize * _bones.binding.setCount);
  _pipelineAnim3D.bindDynamicDS(currentCommandBuffer, &_bonesds, _frameI,  offset);
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
  float deviceRatio = (float)swapchain->offscreenExtent.width /
                  (float)swapchain->offscreenExtent.height;
  float virtualRatio = _targetResolution.x / _targetResolution.y;
  float xCorrection = swapchain->offscreenExtent.width / _targetResolution.x;
  float yCorrection = swapchain->offscreenExtent.height / _targetResolution.y;

  if (virtualRatio < deviceRatio) {
    correction = yCorrection;
  } else {
    correction = xCorrection;
  }
  VP2DData.proj = glm::ortho(
      0.0f, (float)swapchain->offscreenExtent.width*_scale2D / correction, 0.0f,
      (float)swapchain->offscreenExtent.height*_scale2D / correction, -10.0f, 10.0f);
  VP2DData.view = glm::mat4(1.0f);

  VP2D->bindings[0].storeSetData(_frameI, &VP2DData, 0, 0, 0);

  _pipeline2D.begin(currentCommandBuffer, _frameI);
}

void Render::DrawQuad(Resource::Texture texture, glm::mat4 modelMatrix, glm::vec4 colour, glm::vec4 texOffset)
{
  if (_current2DInstanceIndex >= MAX_2D_INSTANCE)
  {
      LOG("WARNING: ran out of 2D instance models!\n");
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
         _modelLoader->drawModel(currentCommandBuffer,
                            _pipeline3D.layout, _currentModel, _modelRuns,
                            _current3DInstanceIndex);
      _current3DInstanceIndex += _modelRuns;
      _modelRuns = 0;
      break;
    case RenderState::Draw2D:
      _modelLoader->drawQuad(currentCommandBuffer,
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
    throw std::runtime_error("Tried to end draw before starting it");

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
      perFrame3D->bindings[0].storeSetData(_frameI, &perFrame3DData[i], 0, i, 0);
      //  perFrame3D->bindings[0].storeData(_frameI, 0, i);
  
  _current3DInstanceIndex = 0;


  for (size_t i = 0; i < _current2DInstanceIndex; i++)
  {
    _per2Dvert.storeData(_frameI, 0, i);
    _per2Dfrag.storeData(_frameI, 0, i);
  }
  _current2DInstanceIndex = 0;


  //FINAL RENDER  PASS
  
  swapchain->endOffscreenRenderPassAndBeginFinal();

  _offscreenTransform.storeData(_frameI);

  _pipelineFinal.begin(currentCommandBuffer, _frameI);

  vkCmdDraw(currentCommandBuffer, 3, 1, 0, 0);

  VkResult result = swapchain->endFinalRenderPass();
  
  if (swapchainRecreationRequired(result) || _framebufferResized) {
      _resize();
  } else if (result != VK_SUCCESS)
      throw std::runtime_error(
	      GET_ERR_STRING("failed to present swapchain image to queue", result));

  submit = true;
}

void Render::_update3DProjectionMatrix() {
  VP3DData.proj =
      glm::perspective(glm::radians(_projectionFov),
                       ((float)swapchain->offscreenExtent.width) /
                           ((float)swapchain->offscreenExtent.height),
                       0.1f, 1000.0f);
  VP3DData.proj[1][1] *=
      -1; // opengl has inversed y axis, so need to correct
}

//recreates frame resources, so any state change for rendering will be updated on next draw if this is called
void Render::FramebufferResize() { _framebufferResized = true; }

void Render::set3DViewMatrixAndFov(glm::mat4 view, float fov, glm::vec4 camPos) {
  VP3DData.view = view;
  _projectionFov = fov;
  _lighting.data[0].camPos = camPos;
  _update3DProjectionMatrix();
}

void Render::set2DViewMatrixAndScale(glm::mat4 view, float scale)
{
  VP2DData.view = view;
  _scale2D = scale;
}

void Render::setLightDirection(glm::vec4 lightDir)
{
  _lightDirection = lightDir;
}

void Render::setForceTargetRes(bool force) {
    if(renderConf.force_target_resolution != force) {
      renderConf.force_target_resolution = force;
      FramebufferResize();
    }
}
bool Render::isTargetResForced() { return renderConf.force_target_resolution; }
void Render::setTargetResolution(glm::vec2 resolution) {
    _targetResolution = resolution;
    renderConf.force_target_resolution = true;
    FramebufferResize();
}
glm::vec2 Render::getTargetResolution() {
  return _targetResolution;
}
void Render::setVsync(bool vsync) {
    this->renderConf.vsync = vsync;
    FramebufferResize();
}

}//namespace
