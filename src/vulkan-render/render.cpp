#include "render.h"
#include "vulkan/vulkan_core.h"

Render::Render(GLFWwindow *window) {
  _initRender(window);
  targetResolution = glm::vec2(mSwapchain.swapchainExtent.width,
                               mSwapchain.swapchainExtent.height);
}

Render::Render(GLFWwindow *window, glm::vec2 target) {
  _initRender(window);
  targetResolution = target;
}

void Render::_initRender(GLFWwindow *window) {
  mWindow = window;
  initVulkan::Instance(&mInstance);
#ifndef NDEBUG
  initVulkan::DebugMessenger(mInstance, &mDebugMessenger);
#endif
  if (glfwCreateWindowSurface(mInstance, mWindow, nullptr, &mSurface) !=
      VK_SUCCESS)
    throw std::runtime_error("failed to create window surface!");
  initVulkan::Device(mInstance, mBase.physicalDevice, &mBase.device, mSurface,
                     &mBase.queue);
  // create general command pool
  VkCommandPoolCreateInfo commandPoolInfo{
      VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  commandPoolInfo.queueFamilyIndex = mBase.queue.graphicsPresentFamilyIndex;

  if (vkCreateCommandPool(mBase.device, &commandPoolInfo, nullptr,
                          &mGeneralCommandPool) != VK_SUCCESS)
    throw std::runtime_error("failed to create command pool");

  // create transfer command buffer
  VkCommandBufferAllocateInfo commandBufferInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  commandBufferInfo.commandPool = mGeneralCommandPool;
  commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  commandBufferInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(mBase.device, &commandBufferInfo,
                               &mTransferCommandBuffer))
    throw std::runtime_error("failed to allocate command buffer");

  mModelLoader = new Resource::ModelRender(mBase, mGeneralCommandPool);
  mTextureLoader = new Resource::TextureLoader(mBase, mGeneralCommandPool);
  mFontLoader = new Resource::FontLoader();
  mTextureLoader->LoadTexture("textures/error.png");
}

Render::~Render() {
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
  initVulkan::DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger,
                                            nullptr);
#endif
  vkDestroyInstance(mInstance, nullptr);
}

void Render::_initFrameResources() {
  initVulkan::Swapchain(mBase.device, mBase.physicalDevice, mSurface,
                        &mSwapchain, mWindow,
                        mBase.queue.graphicsPresentFamilyIndex);

  initVulkan::RenderPass(mBase.device, &mRenderPass, mSwapchain, false);
  initVulkan::RenderPass(mBase.device, &mFinalRenderPass, mSwapchain, true);
  initVulkan::Framebuffers(mBase.device, &mSwapchain, mRenderPass, false);
  initVulkan::Framebuffers(mBase.device, &mSwapchain, mFinalRenderPass, true);

  size_t frameCount = mSwapchain.frameData.size();

  // vertex descriptor sets
  mVP3D.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &mVP3Dds,
                       1);
  initVulkan::DescriptorSetAndLayout(mBase.device, mVP3Dds, {&mVP3D.binding},
                                     VK_SHADER_STAGE_VERTEX_BIT, frameCount);

  mVP2D.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &mVP2Dds,
                       1);
  initVulkan::DescriptorSetAndLayout(mBase.device, mVP2Dds, {&mVP2D.binding},
                                     VK_SHADER_STAGE_VERTEX_BIT, frameCount);

  mPerInstance.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                              &mPerInstance3Dds, MAX_3D_INSTANCE);
  initVulkan::DescriptorSetAndLayout(mBase.device, mPerInstance3Dds,
                                     {&mPerInstance.binding},
                                     VK_SHADER_STAGE_VERTEX_BIT, frameCount);

  mBoneView.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &mBoneViewds,
                           1);
  initVulkan::DescriptorSetAndLayout(mBase.device, mBoneViewds,
                                     {&mBoneView.binding},
                                     VK_SHADER_STAGE_VERTEX_BIT, frameCount);

  mPer2Dvert.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                            &mPer2DVertds, MAX_2D_INSTANCE);
  initVulkan::DescriptorSetAndLayout(mBase.device, mPer2DVertds,
                                     {&mPer2Dvert.binding},
                                     VK_SHADER_STAGE_VERTEX_BIT, frameCount);

  // fragment descriptor sets
  mLighting.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                           &mLightingds, 1);
  initVulkan::DescriptorSetAndLayout(mBase.device, mLightingds,
                                     {&mLighting.binding},
                                     VK_SHADER_STAGE_FRAGMENT_BIT, frameCount);

  mTextureSampler.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_SAMPLER,
                                 &mTexturesds, 1, nullptr,
                                 mTextureLoader->getSamplerP());
  mTextureViews.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                               &mTexturesds, Resource::MAX_TEXTURES_SUPPORTED,
                               mTextureLoader->getImageViewsP(), nullptr);
  initVulkan::DescriptorSetAndLayout(
      mBase.device, mTexturesds,
      {&mTextureSampler.binding, &mTextureViews.binding},
      VK_SHADER_STAGE_FRAGMENT_BIT, frameCount);

  mPer2Dfrag.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                            &mPer2Dfragds, MAX_2D_INSTANCE);
  initVulkan::DescriptorSetAndLayout(mBase.device, mPer2Dfragds,
                                     {&mPer2Dfrag.binding},
                                     VK_SHADER_STAGE_FRAGMENT_BIT, frameCount);

  // temp, move somewhere else later

  VkPhysicalDeviceProperties deviceProps{};
  vkGetPhysicalDeviceProperties(mBase.physicalDevice, &deviceProps);
  VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = samplerInfo.addressModeU;
  samplerInfo.addressModeW = samplerInfo.addressModeU;
  if (settings::PIXELATED) {
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
  } else {
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
  }
  samplerInfo.maxAnisotropy = 1.0f;
  samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.maxLod = 1.0f;
  samplerInfo.minLod = 0.0f;
  if (vkCreateSampler(mBase.device, &samplerInfo, nullptr, &offscreenSampler) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed create sampler");

  // end

  mOffscreenSampler.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_SAMPLER,
                                   &mOffscreends, 1, nullptr,
                                   &offscreenSampler);
  mOffscreenView.setBufferProps(frameCount, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                &mOffscreends, 1, &mSwapchain.offscreen.view,
                                nullptr);
  initVulkan::DescriptorSetAndLayout(
      mBase.device, mOffscreends,
      {&mOffscreenSampler.binding, &mOffscreenView.binding},
      VK_SHADER_STAGE_FRAGMENT_BIT, frameCount);

  initVulkan::PrepareShaderBufferSets(
      mBase,
      {&mVP3D.binding, &mVP2D.binding, &mPerInstance.binding, &mBoneView.binding,
       &mPer2Dvert.binding, &mLighting.binding, &mTextureSampler.binding,
       &mTextureViews.binding, &mPer2Dfrag.binding, &mOffscreenSampler.binding,
       &mOffscreenView.binding},
      &mShaderBuffer, &mShaderMemory);

  initVulkan::GraphicsPipeline(
      mBase.device, &mPipeline3D, mSwapchain, mRenderPass,
      {&mVP3Dds, &mPerInstance3Dds, &mTexturesds, &mLightingds},
      {{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vectPushConstants)},
       {VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(vectPushConstants),
        sizeof(fragPushConstants)}},
      "shaders/v3D-lighting.spv", "shaders/fblinnphong.spv", true, false,
      Vertex3D::attributeDescriptions(), Vertex3D::bindingDescriptions()
);

  initVulkan::GraphicsPipeline(
    mBase.device, &mPipelineAnim3D, mSwapchain, mRenderPass,
    {&mVP3Dds, &mPerInstance3Dds, &mBoneViewds, &mTexturesds, &mLightingds},
    { {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vectPushConstants)},
      {VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(vectPushConstants), sizeof(fragPushConstants)}},
    "shaders/v3D-lighting-anim.spv", "shaders/fblinnphong-anim.spv", true, false,
    VertexAnim3D::attributeDescriptions(), VertexAnim3D::bindingDescriptions()
  );

  initVulkan::GraphicsPipeline(
      mBase.device, &mPipeline2D, mSwapchain, mRenderPass,
      {&mVP2Dds, &mPer2DVertds, &mTexturesds, &mPer2Dfragds},
      {{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vectPushConstants)},
       {VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(vectPushConstants),
        sizeof(fragPushConstants)}},
      "shaders/vflat.spv", "shaders/fflat.spv", true, false,
      Vertex2D::attributeDescriptions(), Vertex2D::bindingDescriptions());

  initVulkan::GraphicsPipeline(mBase.device, &mPipelineFinal, mSwapchain,
                               mFinalRenderPass, {&mOffscreends}, {},
                               "shaders/vfinal.spv", "shaders/ffinal.spv",
                               false, true,
                               Vertex2D::attributeDescriptions(), Vertex2D::bindingDescriptions());

  _updateViewProjectionMatrix();
  mVP2D.data[0].view = glm::mat4(1.0f);
  for (size_t i = 0; i < MAX_3D_INSTANCE; i++) {
    mPerInstance.data[i].model = glm::mat4(1.0f);
    mPerInstance.data[i].normalMat = glm::mat4(1.0f);
  }
  for (size_t i = 0; i < MAX_2D_INSTANCE; i++) {
    mPer2Dvert.data[i].model = glm::mat4(1.0f);
    mPer2Dfrag.data[i].colour = glm::vec4(1.0f);
    mPer2Dfrag.data[i].texOffset = glm::vec4(0, 0, 1, 1);
    mPer2Dfrag.data[i].texID = 0;
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
  mBoneViewds.destroySet(mBase.device);
  mLightingds.destroySet(mBase.device);
  mTexturesds.destroySet(mBase.device);
  mPer2Dfragds.destroySet(mBase.device);
  mOffscreends.destroySet(mBase.device);

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

Resource::Model Render::LoadModel(std::string filepath) {
  if (mFinishedLoadingResources)
    throw std::runtime_error("resource loading has finished already");
  return mModelLoader->loadModel(filepath, mTextureLoader);
}

void Render::EndResourceLoad() {
  mFinishedLoadingResources = true;
  mTextureLoader->endLoading();
  mModelLoader->endLoading(mTransferCommandBuffer);
  _initFrameResources();
}

void Render::_resize() {
  vkDeviceWaitIdle(mBase.device);

  _destroyFrameResources();
  _initFrameResources();

  vkDeviceWaitIdle(mBase.device);
  _updateViewProjectionMatrix();
}

void Render::_startDraw() {
  if (!mFinishedLoadingResources)
    throw std::runtime_error(
        "resource loading must be finished before drawing to screen!");
  mBegunDraw = true;
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
  renderPassInfo.clearValueCount = clearColours.size();
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

  mBoneView.storeData(mImg);
  mPipelineAnim3D.begin(mSwapchain.frameData[mImg].commandBuffer, mImg);
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

void Render::_drawBatch() {
  // std::cout << "batch" << std::endl;
  vectPushConstants vps{glm::mat4(1.0f), glm::mat4(0.0f)};
  vkCmdPushConstants(mSwapchain.frameData[mImg].commandBuffer,
                     mPipeline3D.layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                     sizeof(vectPushConstants), &vps);

  switch(renderState)
  {
    case RenderState::Draw3D:
    case RenderState::DrawAnim3D:
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

void Render::DrawModel(Resource::Model model, glm::mat4 modelMatrix,glm::mat4 normalMat)
{
  if (current3DInstanceIndex >= MAX_3D_INSTANCE) {
#ifndef NDEBUG
    std::cout << "single" << std::endl;
#endif
    vectPushConstants vps{modelMatrix, normalMat};
    vps.normalMat[3][3] = 1.0;
    vkCmdPushConstants(mSwapchain.frameData[mImg].commandBuffer,
                       mPipeline3D.layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(vectPushConstants), &vps);

    mModelLoader->drawModel(mSwapchain.frameData[mImg].commandBuffer,
                            mPipeline3D.layout, model, 1, 0);
    return;
  }

  if (currentModel.ID != model.ID && modelRuns != 0)
    _drawBatch();

  currentModel = model;
  mPerInstance.data[current3DInstanceIndex + modelRuns].model = modelMatrix;
  mPerInstance.data[current3DInstanceIndex + modelRuns].normalMat = normalMat;
  modelRuns++;

  if (current3DInstanceIndex + modelRuns == MAX_3D_INSTANCE)
    _drawBatch();
}

void Render::DrawQuad(Resource::Texture texture, glm::mat4 modelMatrix,
                      glm::vec4 colour, glm::vec4 texOffset) {
  if (current2DInstanceIndex >= MAX_2D_INSTANCE) {
#ifndef NDEBUG
    std::cout << "single" << std::endl;
#endif
    vectPushConstants vps{modelMatrix, glm::mat4(1.0f)};

    vps.normalMat[3][3] = 1.0;
    vkCmdPushConstants(mSwapchain.frameData[mImg].commandBuffer,
                       mPipeline3D.layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(vectPushConstants), &vps);

    mModelLoader->drawQuad(mSwapchain.frameData[mImg].commandBuffer,
                           mPipeline3D.layout, texture.ID, 1, 0, colour,
                           texOffset);
    return;
  }

  mPer2Dvert.data[current2DInstanceIndex + instance2Druns].model = modelMatrix;
  mPer2Dfrag.data[current2DInstanceIndex + instance2Druns].colour = colour;
  mPer2Dfrag.data[current2DInstanceIndex + instance2Druns].texOffset =
      texOffset;
  mPer2Dfrag.data[current2DInstanceIndex + instance2Druns].texID = texture.ID;
  instance2Druns++;

  if (current2DInstanceIndex + instance2Druns == MAX_2D_INSTANCE)
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

void Render::EndDraw(std::atomic<bool> &submit) {
  if (!mBegunDraw)
    throw std::runtime_error("start draw before ending it");
  mBegunDraw = false;

  switch(renderState)
  {
    case RenderState::Draw3D:
    case RenderState::DrawAnim3D:
      if (modelRuns != 0 && current3DInstanceIndex < MAX_3D_INSTANCE)
        _drawBatch();
      break;
    case RenderState::Draw2D:
      if (instance2Druns != 0 && current2DInstanceIndex < MAX_2D_INSTANCE)
        _drawBatch();
      break;
  }

  for (size_t i = 0; i < current3DInstanceIndex; i++)
    mPerInstance.storeData(mImg, i);
  current3DInstanceIndex = 0;

  for (size_t i = 0; i < current2DInstanceIndex; i++) {
    mPer2Dvert.storeData(mImg, i);
    mPer2Dfrag.storeData(mImg, i);
  }
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
  renderPassInfo.clearValueCount = clearColours.size();
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
  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.waitSemaphoreCount = submitWaitSemaphores.size();
  submitInfo.pWaitSemaphores = submitWaitSemaphores.data();
  submitInfo.pWaitDstStageMask = waitStages.data();
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &mSwapchain.frameData[mImg].commandBuffer;
  submitInfo.signalSemaphoreCount = submitSignalSemaphores.size();
  submitInfo.pSignalSemaphores = submitSignalSemaphores.data();
  if (vkQueueSubmit(mBase.queue.graphicsPresentQueue, 1, &submitInfo,
                    mSwapchain.frameData[mImg].frameFinishedFen) != VK_SUCCESS)
    throw std::runtime_error("failed to submit draw command buffer");

  // submit present command
  VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  presentInfo.waitSemaphoreCount = submitSignalSemaphores.size();
  presentInfo.pWaitSemaphores = submitSignalSemaphores.data();
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &mSwapchain.swapChain;
  presentInfo.pImageIndices = &mImg;
  presentInfo.pResults = nullptr;

  // most of draw call time spent here!
  VkResult result =
      vkQueuePresentKHR(mBase.queue.graphicsPresentQueue, &presentInfo);

  if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR ||
      mFramebufferResized) {
    mFramebufferResized = false;
    _resize();
  } else if (result != VK_SUCCESS)
    throw std::runtime_error("failed to present swapchain image to queue");

  mSwapchain.imageAquireSem.push_back(mImgAquireSem);

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
