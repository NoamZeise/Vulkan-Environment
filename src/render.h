#ifndef RENDER_H
#define RENDER_H

#define VOLK_IMPLIMENTATION
#include <volk.h>
#include <GLFW/glfw3.h>
#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>

#include <resources/resources.h>

namespace Resource {
    class ModelRender;
    class FontLoader;
    class TextureLoader;
} // namespace Resource

#include "render_structs/swapchain/swapchain.h"
#include "pipeline.h"
#include "vulkan_manager.h"
#include "render_config.h"
#include "shader.h"
#include "shader_internal.h"
#include "shader_structs.h"

#include <atomic>
#include <vector>

namespace vkenv {

const size_t MAX_ANIMATIONS_PER_FRAME = 10;
const int MAX_3D_INSTANCE = 20;
const int MAX_2D_INSTANCE = 20;

  class Render {
  public:
      Render(GLFWwindow *window, glm::vec2 target);
      ~Render();
      static bool LoadVulkan();
  
      Resource::Texture LoadTexture(std::string filepath);
      Resource::Font LoadFont(std::string filepath);
      Resource::Model LoadModel(std::string filepath);
      Resource::Model LoadAnimatedModel(std::string filepath, std::vector<Resource::ModelAnimation> *pGetAnimations);
      void LoadResourcesToGPU();
      void UseLoadedResources();

      void Begin3DDraw();
      void BeginAnim3DDraw();
      void Begin2DDraw();
      void DrawModel(    Resource::Model model, glm::mat4 modelMatrix, glm::mat4 normalMatrix);
      void DrawAnimModel(Resource::Model model, glm::mat4 modelMatrix, glm::mat4 normalMatrix,
			 Resource::ModelAnimation *animation);
      void DrawQuad(Resource::Texture texture, glm::mat4 modelMatrix, glm::vec4 colour,
		    glm::vec4 texOffset);
      void DrawQuad(Resource::Texture texture, glm::mat4 modelMatrix, glm::vec4 colour);
      void DrawQuad(Resource::Texture texture, glm::mat4 modelMatrix);
      void DrawString(Resource::Font font, std::string text, glm::vec2 position, float size,
		      float depth, glm::vec4 colour, float rotate);
      void DrawString(Resource::Font font, std::string text, glm::vec2 position, float size,
		      float depth, glm::vec4 colour);
      float MeasureString(Resource::Font font, std::string text, float size);
      void EndDraw(std::atomic<bool> &submit);

      void FramebufferResize();
    
      void set3DViewMatrixAndFov(glm::mat4 view, float fov, glm::vec4 camPos);
      void set2DViewMatrixAndScale(glm::mat4 view, float scale);
      void setLightDirection(glm::vec4 lightDir);
      void setForceTargetRes(bool force);
      bool isTargetResForced();
      void setTargetResolution(glm::vec2 resolution);
      glm::vec2 getTargetResolution();
      void setVsync(bool vsync);
      bool getVsync();

      void setTime(float time) {
	  timeData.time = time;
      }
    
  private:
      bool _framebufferResized = false;

      //render settings
      glm::vec2 _targetResolution;
      RenderConfig renderConf;
      bool renderConfChanged = true;

  
      VulkanManager* manager;
  
      Swapchain *swapchain;
      VkCommandBuffer currentCommandBuffer;
  

      Pipeline _pipeline3D;
      Pipeline _pipelineAnim3D;
      Pipeline _pipeline2D;
      Pipeline _pipelineFinal;

      // descriptor set members
      VkDeviceMemory _shaderMemory;
      VkBuffer _shaderBuffer;

      VkDescriptorPool _descPool;

      shaderStructs::timeUbo timeData;
      DescSet *VP3D;
      shaderStructs::viewProjection VP3DData;
      DescSet *VP2D;
      shaderStructs::viewProjection VP2DData;
      DescSet *perFrame3D;
      shaderStructs::PerFrame3D perFrame3DData[MAX_3D_INSTANCE];
      DescSet *bones;
      size_t currentBonesDynamicOffset;
      DescSet *perFrame2DVert;
      glm::mat4 perFrame2DVertData[MAX_2D_INSTANCE];
      DescSet *perFrame2DFrag;
      shaderStructs::Frag2DData perFrame2DFragData[MAX_2D_INSTANCE];
      DescSet *lighting;
      shaderStructs::Lighting lightingData;
      DescSet *offscreenTransform;
      glm::mat4 offscreenTransformData;
      DescSet *textures;
      DescSet *emptyDS;
      DescSet *offscreenTex;
      bool samplerCreated = false;
      VkSampler _offscreenTextureSampler;

      std::vector<DescSet*> descriptorSets;

      //std::vector<descriptor::Set> ds3D;
  
      Resource::TextureLoader *_stagingTextureLoader;
      Resource::ModelRender *_stagingModelLoader;
      Resource::FontLoader *_stagingFontLoader;    
      Resource::TextureLoader *_textureLoader = nullptr;
      Resource::ModelRender *_modelLoader = nullptr;
      Resource::FontLoader *_fontLoader = nullptr;

      enum class RenderState
	  {
	      Draw2D,
	      Draw3D,
	      DrawAnim3D,
	  };

      bool _begunDraw = false;
      RenderState _renderState;
      uint32_t _frameI;
      VkSemaphore _imgAquireSem;
      float _projectionFov = 45.0f;
      float _scale2D = 1.0f;

      unsigned int _modelRuns = 0;
      unsigned int _current3DInstanceIndex = 0;
      Resource::Model _currentModel;
      Resource::Texture _currentTexture;
      glm::vec4 _currentTexOffset = glm::vec4(0, 0, 1, 1);
      glm::vec4 _currentColour = glm::vec4(1, 1, 1, 1);

      unsigned int _instance2Druns = 0;
      unsigned int _current2DInstanceIndex = 0;

      void _initStagingResourceManagers();
      void _initFrameResources();
      void _destroyFrameResources();
      void _startDraw();
      void _resize();
      void _update3DProjectionMatrix();
      void _drawBatch();
  };

} //namespace

#endif
