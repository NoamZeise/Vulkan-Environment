#ifndef RENDER_H
#define RENDER_H

#define VOLK_IMPLIMENTATION
#include <volk.h>
#include <GLFW/glfw3.h>
#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>

#include <graphics/render.h>
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

class PoolManagerVk;

namespace vkenv {

const size_t MAX_ANIMATIONS_PER_FRAME = 10;

  class RenderVk : public Render {
  public:
      /// Try and load Vulkan functions from the installed driver.
      /// Returns whether the operation succeeded or not.
      /// If the operation failed, don't try to create an object of type Render.
      /// This will be called by Render automatically if not called before Render is created.
      static bool LoadVulkan();

      RenderVk(GLFWwindow *window, RenderConfig renderConf);
      ~RenderVk();

      ResourcePool* CreateResourcePool() override;
      void DestroyResourcePool(Resource::Pool pool) override;
      void setResourcePoolInUse(Resource::Pool pool, bool usePool) override;
      ResourcePool* pool(Resource::Pool pool) override;

      void LoadResourcesToGPU(Resource::Pool pool) override;
      void UseLoadedResources() override;

      // warning: switching between models that are in different pools often is slow
      void DrawModel(Resource::Model model, glm::mat4 modelMatrix, glm::mat4 normalMatrix) override;
      void DrawAnimModel(Resource::Model model, glm::mat4 modelMatrix, glm::mat4 normalMatrix,
			 Resource::ModelAnimation *animation) override;
      void DrawQuad(Resource::Texture texture, glm::mat4 modelMatrix, glm::vec4 colour,
		    glm::vec4 texOffset) override;
      void DrawString(Resource::Font font, std::string text, glm::vec2 position, float size,
		      float depth, glm::vec4 colour, float rotate) override;
      void EndDraw(std::atomic<bool> &submit) override;

      void FramebufferResize() override;

      void set3DViewMat(glm::mat4 view, glm::vec4 camPos) override;
      void set2DViewMat(glm::mat4 view) override;
      void set3DProjMat(glm::mat4 proj) override;
      void set2DProjMat(glm::mat4 proj) override;
      void setLightingProps(BPLighting lighting) override;
      void setRenderConf(RenderConfig renderConf) override;
      RenderConfig getRenderConf() override;
      glm::vec2 offscreenSize() override;

      void setTime(float time) {
	  timeData.time = time;
      }
    
  private:
      enum class RenderState {
	  Draw2D,
	  Draw3D,
	  DrawAnim3D,
      };
      
      void _initFrameResources();
      void _destroyFrameResources();
      void _startDraw();
      void _begin(RenderState state);
      void _store3DsetData();
      void _store2DsetData();
      void _resize();
      void _drawBatch();
      void _bindModelPool(Resource::Model model);
      bool _validPool(Resource::Pool pool);
      bool _poolInUse(Resource::Pool pool);
      void _throwIfPoolInvaid(Resource::Pool pool);
      void _loadActiveTextures();
      
      
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
      shaderStructs::PerFrame3D perFrame3DData[Resource::MAX_3D_BATCH];
      DescSet *bones;
      size_t currentBonesDynamicOffset;
      DescSet *perFrame2DVert;
      glm::mat4 perFrame2DVertData[Resource::MAX_2D_BATCH];
      DescSet *perFrame2DFrag;
      shaderStructs::Frag2DData perFrame2DFragData[Resource::MAX_2D_BATCH];
      DescSet *lighting;
      BPLighting lightingData;
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

      std::vector<DescSet*> descriptorSets;

      Resource::Pool defaultPool;
      PoolManagerVk* pools;

      bool _begunDraw = false;
      RenderState _renderState;
      VkSemaphore _imgAquireSem;

      unsigned int _modelRuns = 0;
      unsigned int _current3DInstanceIndex = 0;
      Resource::Model _currentModel;
      Resource::Texture _currentTexture;
      glm::vec4 _currentTexOffset = glm::vec4(0, 0, 1, 1);
      glm::vec4 _currentColour = glm::vec4(1, 1, 1, 1);

      unsigned int _instance2Druns = 0;
      unsigned int _current2DInstanceIndex = 0;

      Resource::Pool currentModelPool;
  };

} //namespace

#endif
