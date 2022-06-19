#include "render.h"
#include "vkinit.h"
#include "vulkan/vulkan_core.h"
#include <stdexcept>
#include <stdint.h>

Render::Render(GLFWwindow *window)
{
  _initRender(window);
  targetResolution = glm::vec2(mSwapchain.swapchainExtent.width,
                               mSwapchain.swapchainExtent.height);
}

Render::Render(GLFWwindow *window, glm::vec2 target)
{
  _initRender(window);
  targetResolution = target;
}

void Render::_initRender(GLFWwindow *window)
{
  mWindow = window;
  initVulkan::Instance(&mInstance);
#ifndef NDEBUG
  initVulkan::DebugMessenger(mInstance, &mDebugMessenger);
#endif
  
  if (glfwCreateWindowSurface(mInstance, mWindow, nullptr, &mSurface) != VK_SUCCESS)
    throw std::runtime_error("failed to create window surface!");
  
  initVulkan::Device(mInstance, &mBase, mSurface);
  
  // create general command pool
  VkCommandPoolCreateInfo commandPoolInfo {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  commandPoolInfo.queueFamilyIndex = mBase.queue.graphicsPresentFamilyIndex;
  if (vkCreateCommandPool(mBase.device, &commandPoolInfo, nullptr, &mGeneralCommandPool) != VK_SUCCESS)
    throw std::runtime_error("failed to create command pool");

  // create transfer command buffer
  VkCommandBufferAllocateInfo commandBufferInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  commandBufferInfo.commandPool = mGeneralCommandPool;
  commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  commandBufferInfo.commandBufferCount = 1;
  if (vkAllocateCommandBuffers(mBase.device, &commandBufferInfo, &mTransferCommandBuffer))
    throw std::runtime_error("failed to allocate command buffer");

  mModelLoader = new Resource::ModelRender(mBase, mGeneralCommandPool);
  mTextureLoader = new Resource::TextureLoader(mBase, mGeneralCommandPool);
  mFontLoader = new Resource::FontLoader();
  mTextureLoader->LoadTexture("textures/error.png");
}

Render::~Render()
{
  vkQueueWaitIdle(mBase.queue.graphicsPresentQueue);
  
  delete mTextureLoader;
  delete mModelLoader;
  delete mFontLoader;
  
  _destroyFrameResources();
  vkDestroyCommandPool(mBase.device, mGeneralCommandPool, nullptr);
  initVulkan::DestroySwapchain(&mSwapchain, mBase.device);
  vkDestroyDevice(mBase.device, nullptr);
  vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
#ifndef NDEBUG
  initVulkan::DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger,nullptr);
#endif
  vkDestroyInstance(mInstance, nullptr);
}

void Render::_initFrameResources()
{
  initVulkan::Swapchain(
      mBase.device,
      mBase.physicalDevice,
      mSurface,
      &mSwapchain,
      mWindow,
      mBase.queue.graphicsPresentFamilyIndex
  );
  size_t frameCount = mSwapchain.frameData.size();

  initVulkan::RenderPass(  mBase.device, &mRenderPass,  mSwapchain, false);
  initVulkan::Framebuffers(mBase.device,  mRenderPass, &mSwapchain, false);
  initVulkan::RenderPass(  mBase.device, &mFinalRenderPass,  mSwapchain, true);
  initVulkan::Framebuffers(mBase.device,  mFinalRenderPass, &mSwapchain, true);

  ///set shader  descripor sets

  ///vertex descripor sets
  
  mVP3D.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &mVP3Dds, 1);
  initVulkan::DescriptorSetLayout(mBase.device, &mVP3Dds, {&mVP3D.binding}, VK_SHADER_STAGE_VERTEX_BIT);

  mVP2D.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &mVP2Dds, 1);
  initVulkan::DescriptorSetLayout(mBase.device, &mVP2Dds, {&mVP2D.binding}, VK_SHADER_STAGE_VERTEX_BIT);

  mPerInstance.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                              &mPerInstance3Dds, 1);
  initVulkan::DescriptorSetLayout(mBase.device, &mPerInstance3Dds, {&mPerInstance.binding}, VK_SHADER_STAGE_VERTEX_BIT);

  mBones.setDynamicBufferProps(frameCount, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, &mBonesds, 1, MAX_ANIMATIONS_PER_FRAME);
  initVulkan::DescriptorSetLayout(mBase.device, &mBonesds, { &mBones.binding }, VK_SHADER_STAGE_VERTEX_BIT);


  mPer2Dvert.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                            &mPer2DVertds, 1);
  initVulkan::DescriptorSetLayout(mBase.device, &mPer2DVertds, {&mPer2Dvert.binding}, VK_SHADER_STAGE_VERTEX_BIT);

  // fragment descriptor sets
  
  mLighting.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &mLightingds, 1);
  initVulkan::DescriptorSetLayout(mBase.device, &mLightingds, {&mLighting.binding}, VK_SHADER_STAGE_FRAGMENT_BIT);

  mTextureSampler.setSamplerBufferProps(frameCount, VK_DESCRIPTOR_TYPE_SAMPLER,
                                 &mTexturesds, 1,
                                 mTextureLoader->getSamplerP());
  mTextureViews.setImageViewBufferProps(frameCount, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                               &mTexturesds, Resource::MAX_TEXTURES_SUPPORTED,
                               mTextureLoader->getImageViewsP());
  initVulkan::DescriptorSetLayout(mBase.device, &mTexturesds, {&mTextureSampler.binding, &mTextureViews.binding}, VK_SHADER_STAGE_FRAGMENT_BIT);


  mPer2Dfrag.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                            &mPer2Dfragds, 1);
  initVulkan::DescriptorSetLayout(mBase.device, &mPer2Dfragds, {&mPer2Dfrag.binding}, VK_SHADER_STAGE_FRAGMENT_BIT);
  
  // render offscreen to tex, then  appy  to quad in final shader
  offscreenSampler = vkhelper::createTextureSampler(mBase.device, mBase.physicalDevice, 1.0f, false);

  mOffscreenSampler.setSamplerBufferProps(
       frameCount,
       VK_DESCRIPTOR_TYPE_SAMPLER,
       &mOffscreends, 1,
       &offscreenSampler
  );
  mOffscreenView.setImageViewBufferProps(
       frameCount,
       VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
       &mOffscreends,
       1,
       &mSwapchain.offscreen.view
  );
  initVulkan::DescriptorSetLayout(mBase.device, &mOffscreends, {&mOffscreenSampler.binding, &mOffscreenView.binding}, VK_SHADER_STAGE_FRAGMENT_BIT);

  //create descripor pool

  initVulkan::DescriptorPoolAndSet(mBase.device, &mDescPool,
                             {
                               &mVP3Dds,
                               &mVP2Dds,
                               &mPerInstance3Dds,
                               &mBonesds,
                               &mPer2DVertds,
                               &mLightingds,
                               &mTexturesds,
                               &mPer2Dfragds,
                               &mOffscreends
                             }
                             , static_cast<uint32_t>(frameCount));

  // create memory mapped buffer for all descriptor set bindings
  initVulkan::PrepareShaderBufferSets(
      mBase,
      {
        &mVP3D.binding,
        &mVP2D.binding,
        &mPerInstance.binding,
        &mBones.binding,
        &mPer2Dvert.binding,
        &mLighting.binding,
        &mTextureSampler.binding,
        &mTextureViews.binding,
        &mPer2Dfrag.binding,
        &mOffscreenSampler.binding,
        &mOffscreenView.binding
      },
      &mShaderBuffer, &mShaderMemory
  );

  // create pipeline for each shader set -> 3D, animated 3D, 2D, and final
  
  initVulkan::GraphicsPipeline(
      mBase.device, &mPipeline3D, mSwapchain, mRenderPass,
      {&mVP3Dds, &mPerInstance3Dds, &mTexturesds, &mLightingds},{
       {VK_SHADER_STAGE_FRAGMENT_BIT, 0,
        sizeof(fragPushConstants)}},
      "shaders/3D-lighting.vert.spv", "shaders/blinnphong.frag.spv", true, false,
      Vertex3D::attributeDescriptions(), Vertex3D::bindingDescriptions()
  );

  initVulkan::GraphicsPipeline(
    mBase.device, &mPipelineAnim3D, mSwapchain, mRenderPass,
    {&mVP3Dds, &mPerInstance3Dds, &mBonesds, &mTexturesds, &mLightingds},
    {
      {VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(fragPushConstants)}},
    "shaders/3D-lighting-anim.vert.spv", "shaders/blinnphong-anim.frag.spv", true, false,
    VertexAnim3D::attributeDescriptions(), VertexAnim3D::bindingDescriptions()
  );

  initVulkan::GraphicsPipeline(
      mBase.device, &mPipeline2D, mSwapchain, mRenderPass,
      {&mVP2Dds, &mPer2DVertds, &mTexturesds, &mPer2Dfragds},
      {},
      "shaders/flat.vert.spv", "shaders/flat.frag.spv", true, false,
      Vertex2D::attributeDescriptions(), Vertex2D::bindingDescriptions()
  );

  initVulkan::GraphicsPipeline(mBase.device, &mPipelineFinal, mSwapchain,
                               mFinalRenderPass, {&mOffscreends}, {},
                               "shaders/final.vert.spv", "shaders/final.frag.spv",
                               false, true,
                               Vertex2D::attributeDescriptions(), Vertex2D::bindingDescriptions()
  );


  _updateViewProjectionMatrix();

  //set initial data
  mVP2D.data[0].view = glm::mat4(1.0f);
  for (size_t i = 0; i < DS::ShaderStructs::MAX_3D_INSTANCE; i++) {
    mPerInstance.data[0].data[i].model = glm::mat4(1.0f);
    mPerInstance.data[0].data[i].normalMat = glm::mat4(1.0f);
  }
  for (size_t i = 0; i < DS::ShaderStructs::MAX_2D_INSTANCE; i++) {
    mPer2Dvert.data[0].model[i] = glm::mat4(1.0f);
    mPer2Dfrag.data[0].data[i].colour = glm::vec4(1.0f);
    mPer2Dfrag.data[0].data[i].texOffset = glm::vec4(0, 0, 1, 1);
    mPer2Dfrag.data[0].data[i].texID = 0;
  }
}

void Render::_destroyFrameResources() {
  vkDestroyBuffer(mBase.device, mShaderBuffer, nullptr);
  vkFreeMemory(mBase.device, mShaderMemory, nullptr);

  vkDestroySampler(mBase.device, offscreenSampler, nullptr);

  mVP3Dds.destroySet(mBase.device);
  mVP2Dds.destroySet(mBase.device);
  mPerInstance3Dds.destroySet(mBase.device);
  mPer2DVertds.destroySet(mBase.device);
  mBonesds.destroySet(mBase.device);
  mLightingds.destroySet(mBase.device);
  mTexturesds.destroySet(mBase.device);
  mPer2Dfragds.destroySet(mBase.device);
  mOffscreends.destroySet(mBase.device);

  vkDestroyDescriptorPool(mBase.device, mDescPool, nullptr);

  vkDestroyFramebuffer(mBase.device, mSwapchain.offscreenFramebuffer, nullptr);
  for (size_t i = 0; i < mSwapchain.frameData.size(); i++) {
    vkDestroyFramebuffer(mBase.device, mSwapchain.frameData[i].framebuffer,
                         nullptr);
  }
  mPipeline3D.destroy(mBase.device);
  mPipelineAnim3D.destroy(mBase.device);
  mPipeline2D.destroy(mBase.device);
  mPipelineFinal.destroy(mBase.device);
  vkDestroyRenderPass(mBase.device, mRenderPass, nullptr);
  vkDestroyRenderPass(mBase.device, mFinalRenderPass, nullptr);
}

void Render::restartResourceLoad() { mTextureLoader->UnloadTextures(); }

Resource::Texture Render::LoadTexture(std::string filepath) {
  if (mFinishedLoadingResources)
    throw std::runtime_error("resource loading has finished already");
  return mTextureLoader->LoadTexture(filepath);
}

Resource::Font Render::LoadFont(std::string filepath) {
  if (mFinishedLoadingResources)
    throw std::runtime_error("resource loading has finished already");
  try {
    return mFontLoader->LoadFont(filepath, mTextureLoader);
  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
    return Resource::Font();
  }
}

Resource::Model Render::LoadAnimatedModel(std::string filepath, std::vector<Resource::ModelAnimation> *pGetAnimations)
{
   if (mFinishedLoadingResources)
    throw std::runtime_error("resource loading has finished already");
  return mModelLoader->loadModel(filepath, mTextureLoader, pGetAnimations);
}

Resource::Model Render::LoadModel(std::string filepath)
{
  if (mFinishedLoadingResources)
    throw std::runtime_error("resource loading has finished already");
  return mModelLoader->loadModel(filepath, mTextureLoader);
}

void Render::EndResourceLoad()
{
  mFinishedLoadingResources = true;
  mTextureLoader->endLoading();
  mModelLoader->endLoading(mTransferCommandBuffer);
  _initFrameResources();
}

void Render::_resize()
{
  vkDeviceWaitIdle(mBase.device);

  _destroyFrameResources();
  _initFrameResources();

  vkDeviceWaitIdle(mBase.device);
  _updateViewProjectionMatrix();
}

void Render::_startDraw()
{
  if (!mFinishedLoadingResources)
    throw std::runtime_error(
        "resource loading must be finished before drawing to screen!");
  mBegunDraw = true;

  std::cout << "beginning draw\n";

  if (mSwapchain.imageAquireSem.empty()) {
    VkSemaphoreCreateInfo semaphoreInfo{
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    if (vkCreateSemaphore(mBase.device, &semaphoreInfo, nullptr,
                          &mImgAquireSem) != VK_SUCCESS)
      throw std::runtime_error("failed to create image available semaphore");
  } else {
    mImgAquireSem = mSwapchain.imageAquireSem.back();
    mSwapchain.imageAquireSem.pop_back();
  }
  if (vkAcquireNextImageKHR(mBase.device, mSwapchain.swapChain, UINT64_MAX,
                            mImgAquireSem, VK_NULL_HANDLE,
                            &mImg) != VK_SUCCESS) {
    mSwapchain.imageAquireSem.push_back(mImgAquireSem);
    return;
  }

  if (mSwapchain.frameData[mImg].frameFinishedFen != VK_NULL_HANDLE) {
    vkWaitForFences(mBase.device, 1,
                    &mSwapchain.frameData[mImg].frameFinishedFen, VK_TRUE,
                    UINT64_MAX);
    vkResetFences(mBase.device, 1,
                  &mSwapchain.frameData[mImg].frameFinishedFen);
  }
  vkResetCommandPool(mBase.device, mSwapchain.frameData[mImg].commandPool, 0);

std::cout << "command pool reset\n";

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  beginInfo.pInheritanceInfo = nullptr;

  if (vkBeginCommandBuffer(mSwapchain.frameData[mImg].commandBuffer,
                           &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to being recording command buffer");
  }

  // fill render pass begin struct
  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = mRenderPass;
  renderPassInfo.framebuffer =
      mSwapchain.offscreenFramebuffer; // framebuffer for each swapchain image
                                       // should match size of attachments
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = mSwapchain.offscreenExtent;
  // clear colour -> values for VK_ATTACHMENT_LOAD_OP_CLEAR load operation in
  // colour attachment need colour for each attachment being cleared (colour,
  // depth)
  std::array<VkClearValue, 2> clearColours{};
  clearColours[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearColours[1].depthStencil = {1.0f, 0};
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearColours.size());
  renderPassInfo.pClearValues = clearColours.data();

  vkCmdBeginRenderPass(mSwapchain.frameData[mImg].commandBuffer,
                       &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{
      0.0f,
      0.0f, // x  y
      (float)mSwapchain.offscreenExtent.width,
      (float)mSwapchain.offscreenExtent.height, // width  height
      0.0f,
      1.0f // min/max depth
  };
  VkRect2D scissor{VkOffset2D{0, 0}, mSwapchain.offscreenExtent};
  vkCmdSetViewport(mSwapchain.frameData[mImg].commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(mSwapchain.frameData[mImg].commandBuffer, 0, 1, &scissor);

  mModelLoader->bindBuffers(mSwapchain.frameData[mImg].commandBuffer);
  mBones.currentDynamicOffsetIndex = 0;
}

void Render::Begin3DDraw()
{
  if (!mBegunDraw)
    _startDraw();
  if (modelRuns > 0)
    _drawBatch();
  if (instance2Druns > 0)
    _drawBatch();
  renderState = RenderState::Draw3D;

  mVP3D.storeData(mImg);
  mLighting.data[0].direction =
      glm::transpose(glm::inverse(mVP3D.data[0].view)) *
      glm::vec4(0.3f, -0.3f, -0.5f, 0.0f);
  mLighting.storeData(mImg);

  mPipeline3D.begin(mSwapchain.frameData[mImg].commandBuffer, mImg);
}

void Render::DrawModel(Resource::Model model, glm::mat4 modelMatrix, glm::mat4 normalMat)
{
  if (current3DInstanceIndex >= DS::ShaderStructs::MAX_3D_INSTANCE) {
    std::cout << "WARNING: ran out of 3D instance models!\n";
  }

  if (currentModel.ID != model.ID && modelRuns != 0)
    _drawBatch();

  currentModel = model;
  mPerInstance.data[0].data[current3DInstanceIndex + modelRuns].model = modelMatrix;
  mPerInstance.data[0].data[current3DInstanceIndex + modelRuns].normalMat = normalMat;
  modelRuns++;

  if (current3DInstanceIndex + modelRuns == DS::ShaderStructs::MAX_3D_INSTANCE)
    _drawBatch();
}

void Render::BeginAnim3DDraw()
{
  if (!mBegunDraw)
    _startDraw();
  if (modelRuns > 0)
    _drawBatch();
  if (instance2Druns > 0)
    _drawBatch();
  renderState = RenderState::DrawAnim3D;

  mVP3D.storeData(mImg);
  mLighting.data[0].direction =
      glm::transpose(glm::inverse(mVP3D.data[0].view)) *
      glm::vec4(0.3f, -0.3f, -0.5f, 0.0f);
  mLighting.storeData(mImg);
  mPipelineAnim3D.begin(mSwapchain.frameData[mImg].commandBuffer, mImg);
}

void Render::DrawAnimModel(Resource::Model model, glm::mat4 modelMatrix, glm::mat4 normalMat, Resource::ModelAnimation *animation)
{
   if (current3DInstanceIndex >= DS::ShaderStructs::MAX_3D_INSTANCE) {
     std::cout << "WARNING: Ran out of 3D Anim Instance models!\n";
  }

  if (currentModel.ID != model.ID && modelRuns != 0)
    _drawBatch();

  currentModel = model;
  mPerInstance.data[0].data[current3DInstanceIndex + modelRuns].model = modelMatrix;
  mPerInstance.data[0].data[current3DInstanceIndex + modelRuns].normalMat = normalMat;
  modelRuns++;

  auto bones = animation->getCurrentBones();
  for(int i = 0; i < bones->size() && i < 50; i++)
  {
    mBones.data[0].mat[i] = bones->at(i);
  }
  if(mBones.currentDynamicOffsetIndex >= MAX_ANIMATIONS_PER_FRAME)
  {
    std::cout << "warning, too many animation calls!\n";
    return;
  }
  mBones.storeData(mImg);
  uint32_t offset = static_cast<uint32_t>((mBones.currentDynamicOffsetIndex-1) * mBones.binding.bufferSize * mBones.binding.setCount);
  mPipelineAnim3D.bindDynamicDS(mSwapchain.frameData[mImg].commandBuffer, &mBonesds, mImg,  offset);
   _drawBatch();
}

void Render::Begin2DDraw()
{
  if (!mBegunDraw)
    _startDraw();
  if (modelRuns > 0)
    _drawBatch();
  if (instance2Druns > 0)
    _drawBatch();
  renderState = RenderState::Draw2D;

  float correction;
  float deviceRatio = (float)mSwapchain.offscreenExtent.width /
                  (float)mSwapchain.offscreenExtent.height;
  float virtualRatio = targetResolution.x / targetResolution.y;
  float xCorrection = mSwapchain.offscreenExtent.width / targetResolution.x;
  float yCorrection = mSwapchain.offscreenExtent.height / targetResolution.y;

  if (virtualRatio < deviceRatio) {
    correction = yCorrection;
  } else {
    correction = xCorrection;
  }
  mVP2D.data[0].proj = glm::ortho(
      0.0f, (float)mSwapchain.offscreenExtent.width*scale2D / correction, 0.0f,
      (float)mSwapchain.offscreenExtent.height*scale2D / correction, -10.0f, 10.0f);
  mVP2D.data[0].view = glm::mat4(1.0f);

  mVP2D.storeData(mImg);

  mPipeline2D.begin(mSwapchain.frameData[mImg].commandBuffer, mImg);
}

void Render::DrawQuad(Resource::Texture texture, glm::mat4 modelMatrix, glm::vec4 colour, glm::vec4 texOffset)
{
  if (current2DInstanceIndex >= DS::ShaderStructs::MAX_2D_INSTANCE)
  {
    std::cout << "WARNING: ran out of 2D instance models!\n";
  }

  mPer2Dvert.data[0].model[current2DInstanceIndex + instance2Druns] = modelMatrix;
  mPer2Dfrag.data[0].data[current2DInstanceIndex + instance2Druns].colour = colour;
  mPer2Dfrag.data[0].data[current2DInstanceIndex + instance2Druns].texOffset =
      texOffset;
  mPer2Dfrag.data[0].data[current2DInstanceIndex + instance2Druns].texID = texture.ID;
  instance2Druns++;

  if (current2DInstanceIndex + instance2Druns == DS::ShaderStructs::MAX_2D_INSTANCE)
    _drawBatch();
}

void Render::DrawQuad(Resource::Texture texture, glm::mat4 modelMatrix,
                      glm::vec4 colour) {
  DrawQuad(texture, modelMatrix, colour, glm::vec4(0, 0, 1, 1));
}

void Render::DrawQuad(Resource::Texture texture, glm::mat4 modelMatrix) {
  DrawQuad(texture, modelMatrix, glm::vec4(1), glm::vec4(0, 0, 1, 1));
}

void Render::DrawString(Resource::Font font, std::string text, glm::vec2 position, float size, float depth, glm::vec4 colour, float rotate)
{
  auto draws = mFontLoader->DrawString(font, text, position, size, depth,
                                       colour, rotate);
  for (const auto &draw : draws)
  {
    DrawQuad(draw.tex, draw.model, draw.colour);
  }
}
void Render::DrawString(Resource::Font font, std::string text, glm::vec2 position, float size, float depth, glm::vec4 colour)
{
  DrawString(font, text, position, size, depth, colour, 0.0);
}

float Render::MeasureString(Resource::Font font, std::string text, float size)
{
  return mFontLoader->MeasureString(font, text, size);
}

void Render::_drawBatch()
{

  std::cout << "draw call\n";

  switch(renderState)
  {

       case RenderState::DrawAnim3D:
       case RenderState::Draw3D:
         mModelLoader->drawModel(mSwapchain.frameData[mImg].commandBuffer,
                            mPipeline3D.layout, currentModel, modelRuns,
                            current3DInstanceIndex);
      current3DInstanceIndex += modelRuns;
      modelRuns = 0;
      break;
    case RenderState::Draw2D:
      mModelLoader->drawQuad(mSwapchain.frameData[mImg].commandBuffer,
                           mPipeline3D.layout, 0, instance2Druns,
                           current2DInstanceIndex, currentColour,
                           currentTexOffset);
      current2DInstanceIndex += instance2Druns;
      instance2Druns = 0;
      break;
  }
}

void Render::EndDraw(std::atomic<bool> &submit) {
  if (!mBegunDraw)
    throw std::runtime_error("start draw before ending it");
  mBegunDraw = false;

  switch(renderState)
  {
    case RenderState::Draw3D:
    case RenderState::DrawAnim3D:
      if (modelRuns != 0 && current3DInstanceIndex < DS::ShaderStructs::MAX_3D_INSTANCE)
        _drawBatch();
      break;
    case RenderState::Draw2D:
      if (instance2Druns != 0 && current2DInstanceIndex < DS::ShaderStructs::MAX_2D_INSTANCE)
        _drawBatch();
      break;
  }

  //TODO only send batched data, not all!!!

 // for (size_t i = 0; i < current3DInstanceIndex; i++)
 //   mPerInstance.storeData(mImg, i);
 // current3DInstanceIndex = 0;
 //
 mPerInstance.storeData(mImg, 0);
 current3DInstanceIndex = 0;



  //for (size_t i = 0; i < current2DInstanceIndex; i++) {
  //  mPer2Dvert.storeData(mImg, i);
  //  mPer2Dfrag.storeData(mImg, i);
 // }
 //
  mPer2Dvert.storeData(mImg, 0);
  mPer2Dfrag.storeData(mImg, 0);
  current2DInstanceIndex = 0;


  // end render pass
  vkCmdEndRenderPass(mSwapchain.frameData[mImg].commandBuffer);

  // final render pass

  // fill render pass begin struct
  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = mFinalRenderPass;
  renderPassInfo.framebuffer =
      mSwapchain.frameData[mImg]
          .framebuffer; // framebuffer for each swapchain image
                        // should match size of attachments
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = mSwapchain.swapchainExtent;

  std::array<VkClearValue, 1> clearColours{};
  clearColours[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearColours.size());
  renderPassInfo.pClearValues = clearColours.data();

  vkCmdBeginRenderPass(mSwapchain.frameData[mImg].commandBuffer,
                       &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{
      0.0f,
      0.0f, // x  y
      (float)mSwapchain.swapchainExtent.width,
      (float)mSwapchain.swapchainExtent.height, // width  height
      0.0f,
      1.0f // min/max depth
  };
  VkRect2D scissor{VkOffset2D{0, 0}, mSwapchain.swapchainExtent};
  vkCmdSetViewport(mSwapchain.frameData[mImg].commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(mSwapchain.frameData[mImg].commandBuffer, 0, 1, &scissor);

  mPipelineFinal.begin(mSwapchain.frameData[mImg].commandBuffer, mImg);

  vkCmdDraw(mSwapchain.frameData[mImg].commandBuffer, 3, 1, 0, 0);

  vkCmdEndRenderPass(mSwapchain.frameData[mImg].commandBuffer);

  // final render pass end

  if (vkEndCommandBuffer(mSwapchain.frameData[mImg].commandBuffer) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }

  std::array<VkSemaphore, 1> submitWaitSemaphores = {mImgAquireSem};
  std::array<VkPipelineStageFlags, 1> waitStages = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  std::array<VkSemaphore, 1> submitSignalSemaphores = {
      mSwapchain.frameData[mImg].presentReadySem};

  // submit draw command
  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr};
  submitInfo.waitSemaphoreCount = static_cast<uint32_t>(submitWaitSemaphores.size());
  submitInfo.pWaitSemaphores = submitWaitSemaphores.data();
  submitInfo.pWaitDstStageMask = waitStages.data();
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &mSwapchain.frameData[mImg].commandBuffer;
  submitInfo.signalSemaphoreCount = static_cast<uint32_t>(submitSignalSemaphores.size());
  submitInfo.pSignalSemaphores = submitSignalSemaphores.data();
  if (vkQueueSubmit(mBase.queue.graphicsPresentQueue, 1, &submitInfo,
                    mSwapchain.frameData[mImg].frameFinishedFen) != VK_SUCCESS)
    throw std::runtime_error("failed to submit draw command buffer");

  // submit present command
  VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr};
  presentInfo.waitSemaphoreCount = static_cast<uint32_t>(submitSignalSemaphores.size());
  presentInfo.pWaitSemaphores = submitSignalSemaphores.data();
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &mSwapchain.swapChain;
  presentInfo.pImageIndices = &mImg;
  presentInfo.pResults = nullptr;

  VkResult result =
      vkQueuePresentKHR(mBase.queue.graphicsPresentQueue, &presentInfo);

  if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR ||
      mFramebufferResized) {
    mFramebufferResized = false;
    _resize();
  } else if (result != VK_SUCCESS)
    throw std::runtime_error("failed to present swapchain image to queue");

  mSwapchain.imageAquireSem.push_back(mImgAquireSem);

//TODO remove when segfault fixed
  vkQueueWaitIdle(mBase.queue.graphicsPresentQueue);

  std::cout << "finished draw\n";

  submit = true;
}

void Render::_updateViewProjectionMatrix() {
  mVP3D.data[0].proj =
      glm::perspective(glm::radians(mProjectionFov),
                       ((float)mSwapchain.offscreenExtent.width) /
                           ((float)mSwapchain.offscreenExtent.height),
                       0.1f, 500.0f);
  mVP3D.data[0].proj[1][1] *=
      -1; // opengl has inversed y axis, so need to correct
}

void Render::set3DViewMatrixAndFov(glm::mat4 view, float fov) {
  mVP3D.data[0].view = view;
  mProjectionFov = fov;
  _updateViewProjectionMatrix();
}

void Render::set2DViewMatrixAndScale(glm::mat4 view, float scale)
{
  mVP2D.data[0].view = view;
  scale2D = scale;
}

void Render::FramebufferResize() { mFramebufferResized = true; }
