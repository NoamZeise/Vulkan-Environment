#ifndef RENDER_H
#define RENDER_H

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>
#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <atomic>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

#include <glmhelper.h>

#include "parts/primary.h"
#include "parts/swapchain.h"
#include "parts/render_style.h"
#include "parts/descriptors.h"
#include "parts/images.h"
#include "descriptor_structs.h"
#include "resources/model/model_render.h"
#include "pipeline.h"
#include "render_structs.h"
#include "resources/font_loader.h"
#include "resources/texture_loader.h"
#include "vkhelper.h"

const size_t MAX_ANIMATIONS_PER_FRAME = 10;
const int MAX_3D_INSTANCE = 20;
const int MAX_2D_INSTANCE = 20;

class Render {
public:
  Render(GLFWwindow *window);
  Render(GLFWwindow *window, glm::vec2 target);
  ~Render();
  static void SetGLFWWindowHints() {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  }
  void set3DViewMatrixAndFov(glm::mat4 view, float fov, glm::vec4 camPos);
  void set2DViewMatrixAndScale(glm::mat4 view, float scale);
  void setLightDirection(glm::vec4 lightDir);
  void restartResourceLoad();

  Resource::Texture LoadTexture(std::string filepath);
  Resource::Font LoadFont(std::string filepath);
  Resource::Model LoadModel(std::string filepath);
  Resource::Model LoadAnimatedModel(std::string filepath, std::vector<Resource::ModelAnimation> *pGetAnimations);

  void EndResourceLoad();

  void Begin3DDraw();
  void BeginAnim3DDraw();
  void Begin2DDraw();
  void DrawModel(    Resource::Model model, glm::mat4 modelMatrix, glm::mat4 normalMatrix);
  void DrawAnimModel(Resource::Model model, glm::mat4 modelMatrix, glm::mat4 normalMatrix, Resource::ModelAnimation *animation);
  void DrawQuad(Resource::Texture texture, glm::mat4 modelMatrix, glm::vec4 colour, glm::vec4 texOffset);
  void DrawQuad(Resource::Texture texture, glm::mat4 modelMatrix, glm::vec4 colour);
  void DrawQuad(Resource::Texture texture, glm::mat4 modelMatrix);
  void DrawString(Resource::Font font, std::string text, glm::vec2 position, float size, float depth, glm::vec4 colour, float rotate);
  void DrawString(Resource::Font font, std::string text, glm::vec2 position, float size, float depth, glm::vec4 colour);
  float MeasureString(Resource::Font font, std::string text, float size);
  void EndDraw(std::atomic<bool> &submit);

  void FramebufferResize();

private:
  bool _framebufferResized = false;

  GLFWwindow *_window;
  glm::vec2 _targetResolution;
  VkInstance _instance;
  VkSurfaceKHR _surface;
  Base _base;
  FrameData _frame;
  SwapChain _swapchain;
  VkRenderPass _renderPass;
  VkRenderPass _finalRenderPass;

  VkCommandPool _generalCommandPool;
  VkCommandBuffer _transferCommandBuffer;

  Pipeline _pipeline3D;
  Pipeline _pipelineAnim3D;
  Pipeline _pipeline2D;
  Pipeline _pipelineFinal;

  // descriptor set members
  VkDeviceMemory _shaderMemory;
  VkBuffer _shaderBuffer;

  VkDescriptorPool _descPool;

  DS::DescriptorSet _VP3Dds;
  DS::DescriptorSet _VP2Dds;
  DS::DescriptorSet _perInstance3Dds;
  DS::DescriptorSet _bonesds;
  DS::DescriptorSet _per2DVertds;
  DS::DescriptorSet _lightingds;
  DS::DescriptorSet _texturesds;
  DS::DescriptorSet _per2Dfragds;
  DS::DescriptorSet _offscreends;

  DS::BindingAndData<DS::ShaderStructs::viewProjection> _VP3D;
  DS::BindingAndData<DS::ShaderStructs::viewProjection> _VP2D;
  DS::BindingAndData<DS::ShaderStructs::PerFrame3D> _perInstance;
  DS::BindingAndData<DS::ShaderStructs::Bones> _bones;
  DS::BindingAndData<glm::mat4> _per2Dvert;
  DS::BindingAndData<bool> _textureViews;
  DS::BindingAndData<bool> _textureSampler;
  DS::BindingAndData<DS::ShaderStructs::Frag2DData> _per2Dfrag;
  DS::BindingAndData<DS::ShaderStructs::lighting> _lighting;
  DS::BindingAndData<bool> _offscreenSampler;
  DS::BindingAndData<bool> _offscreenView;
  VkSampler _offscreenTextureSampler;

  Resource::TextureLoader *_textureLoader;
  Resource::ModelRender *_modelLoader;
  Resource::FontLoader *_fontLoader;

  enum class RenderState
  {
    Draw2D,
    Draw3D,
    DrawAnim3D,
  };

  bool _begunDraw = false;
  bool _finishedLoadingResources = false;
  RenderState _renderState;
  uint32_t _frameI;
  VkSemaphore _imgAquireSem;
  float _projectionFov = 45.0f;
  float _scale2D = 1.0f;
  glm::vec4 _lightDirection = glm::vec4(0.3f, 0.3f, -0.5f, 0.0f);

  unsigned int _modelRuns = 0;
  unsigned int _current3DInstanceIndex = 0;
  Resource::Model _currentModel;
  Resource::Texture _currentTexture;
  glm::vec4 _currentTexOffset = glm::vec4(0, 0, 1, 1);
  glm::vec4 _currentColour = glm::vec4(1, 1, 1, 1);

  unsigned int _instance2Druns = 0;
  unsigned int _current2DInstanceIndex = 0;

  void _initRender(GLFWwindow *window);
  void _initFrameResources();
  void _destroyFrameResources();
  void _startDraw();
  void _resize();
  void _updateViewProjectionMatrix();
  void _drawBatch();

#ifndef NDEBUG
  VkDebugUtilsMessengerEXT _debugMessenger;
#endif
};

#endif
