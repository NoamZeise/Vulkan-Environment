#ifndef RENDER_H
#define RENDER_H

#define VOLK_IMPLIMENTATION
#include <volk.h>
#include <GLFW/glfw3.h>
#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>

#include <graphics/resources.h>
#include <graphics/render_config.h>
#include <graphics/shader_structs.h>
#include "vulkan_manager.h"
#include "swapchain.h"
#include "frame.h"
#include "renderpass.h"
#include "pipeline.h"
#include "shader.h"
#include "shader_internal.h"
#include "shader_structs.h"
#include <atomic>
#include <vector>

struct ResourcePool;

namespace vkenv {

const size_t MAX_ANIMATIONS_PER_FRAME = 10;
const int MAX_3D_INSTANCE = 50;
const int MAX_2D_INSTANCE = 500;

  class Render {
  public:
      /// Try and load Vulkan functions from the installed driver.
      /// Returns whether the operation succeeded or not.
      /// If the operation failed, don't try to create an object of type Render.
      /// This will be called by Render automatically if not called before Render is created.
      static bool LoadVulkan();
      /// Iniltialise the renderer. Chooses a GPU and sets up resources loaders.
      /// Do any resource loading, then call LoadResourcesToGPU(), then UseLoadedResources() 
      /// before the draw loop.
      Render(GLFWwindow *window, RenderConfig renderConf);
      ~Render();

      /// Create a new pool to load resource into.
      /// Pool will be in use by default.
      Resource::ResourcePool CreateResourcePool();
      void DestroyResourcePool(Resource::ResourcePool pool);
      /// enable or disable using this resource pool's GPU loaded resources
      /// will only take effect after a frame resource recreation
      /// (such as by calling UseLoadedResources() or if the framebuffer is resized)
      void setResourcePoolInUse(Resource::ResourcePool pool, bool usePool);

      /// Load a 2D image file
      Resource::Texture LoadTexture(std::string filepath);
      Resource::Texture LoadTexture(Resource::ResourcePool pool, std::string filepath);
      Resource::Font LoadFont(std::string filepath);
      /// Load model from the filepath and store as a 2D model.
      /// Can be used in 2D draws, which will be drawn with orthographic projection.
      Resource::Model Load2DModel(std::string filepath);
      /// Load 2D model from a ModelInfo::Model.
      /// For loading your own models that aren't from a model file.
      Resource::Model Load2DModel(ModelInfo::Model& model);
      /// Load a model from a file and store as a 3D model.
      Resource::Model Load3DModel(std::string filepath);
      /// Load 3D model from a ModelInfo::Model.
      /// For loading your own models that aren't from a model file.
      Resource::Model Load3DModel(ModelInfo::Model& model);
      /// Load a model that has animations
      /// Supply a pointer to a vector of ModelAnimations to get the animations that the model has.
      /// If the model has no animations, the model will be loaded as 3D.
      Resource::Model LoadAnimatedModel(std::string filepath, std::vector<Resource::ModelAnimation> *pGetAnimations);
      Resource::Model LoadAnimatedModel(ModelInfo::Model& model,
					std::vector<Resource::ModelAnimation> *pGetAnimation);

      /// Load A pool's resource from CPU to GPU memory
      /// This must be done before the pool's resources can be used.
      void LoadResourcesToGPU();
      void LoadResourcesToGPU(Resource::ResourcePool);
      /// Reload frame resources, using any resources that have been loaded
      /// to the GPU from resource pools, and that have useResourcePool
      /// set to true.
      void UseLoadedResources();

      void Begin3DDraw();
      void BeginAnim3DDraw();
      void Begin2DDraw();
      //switching between models that are in different pools often is slow
      void DrawModel(Resource::Model model, glm::mat4 modelMatrix, glm::mat4 normalMatrix);
      void DrawModel(Resource::Model model, glm::mat4 modelMatrix, glm::mat4 normalMatrix,
		     glm::vec4 modelColour);
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

      void setRenderConf(RenderConfig renderConf);
      RenderConfig getRenderConf();
      void setTargetResolution(glm::vec2 resolution);
      glm::vec2 getTargetResolution();

      void setPalette(ShaderPalette palette);

      void setTime(float time) {
	  timeData.time = time;
      }
    
  private:
      void _initFrameResources();
      void _destroyFrameResources();
      void _startDraw();
      void _resize();
      void _update3DProjectionMatrix();
      void _drawBatch();
      void _bindModelPool(Resource::Model model);
      bool _validPool(Resource::ResourcePool pool);
      bool _poolInUse(Resource::ResourcePool pool);
      void _throwIfPoolInvaid(Resource::ResourcePool pool);

      
      bool _framebufferResized = false;
      bool _frameResourcesCreated = false;

      RenderConfig renderConf;
      RenderConfig prevRenderConf;
  
      VulkanManager* manager = nullptr;
      uint32_t frameIndex = 0;
      const uint32_t frameCount = 2;
      Frame** frames;

      VkFormat offscreenDepthFormat;
      VkFormat prevSwapchainFormat = VK_FORMAT_UNDEFINED;
      VkSampleCountFlagBits prevSampleCount = VK_SAMPLE_COUNT_1_BIT;
      Swapchain *swapchain = nullptr;
      uint32_t swapchainFrameIndex = 0;
      uint32_t swapchainFrameCount = 0;
      VkCommandBuffer currentCommandBuffer = VK_NULL_HANDLE;

      VkDeviceMemory framebufferMemory = VK_NULL_HANDLE;
      RenderPass* offscreenRenderPass = nullptr;
      RenderPass* finalRenderPass = nullptr;

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
      bool offscreenSamplerCreated = false;
      VkSampler _offscreenTextureSampler;
      bool textureSamplerCreated = false;
      float prevTexSamplerMinMipmap = 1.0f;
      VkSampler textureSampler;
      VkImageView textureViews[Resource::MAX_TEXTURES_SUPPORTED];
      
      DescSet *paletteDS;
      ShaderPalette paletteData;

      std::vector<DescSet*> descriptorSets;

      std::vector<ResourcePool*> pools;
      std::vector<int> freePools;

      enum class RenderState {
	  Draw2D,
	  Draw3D,
	  DrawAnim3D,
      };

      bool _begunDraw = false;
      RenderState _renderState;
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

      Resource::ResourcePool currentModelPool;
  };

} //namespace

#endif
