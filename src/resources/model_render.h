#ifndef MODEL_RENDER_H
#define MODEL_RENDER_H

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <map>
#include <string>

#include <resources/resources.h>
#include <vertex_types.h>
#ifndef NO_ASSIMP
#include <resource_loader/model_loader.h>
#endif
#include "texture_loader.h"
#include "../render_structs/device_state.h"
#include <resource_loader/vertex_model.h>

namespace Resource
{
  struct ModelInGPU;
  enum class ModelType;

  class ModelRender
  {
  public:
      ModelRender(DeviceState base, VkCommandPool pool);
      ~ModelRender();
      
      Model load2DModel(std::string path, TextureLoader* texLoader);
      Model load2DModel(ModelInfo::Model& model, TextureLoader* texLoader);
      Model load3DModel(std::string path, TextureLoader* texLoader);
      Model load3DModel(ModelInfo::Model& model, TextureLoader* texLoader);
      Model loadAnimatedModel(std::string path, TextureLoader* texLoader,
			      std::vector<Resource::ModelAnimation> *pGetAnimations);
      Model loadAnimatedModel(ModelInfo::Model& model, TextureLoader* texLoader,
			      std::vector<Resource::ModelAnimation> *pGetAnimations);
      void endLoading(VkCommandBuffer transferBuff);

      void bindBuffers(VkCommandBuffer cmdBuff);
      void drawModel(VkCommandBuffer cmdBuff, VkPipelineLayout layout, Model model,
		     uint32_t count, uint32_t instanceOffset);
      void drawQuad(VkCommandBuffer cmdBuff, VkPipelineLayout layout, unsigned int texID,
		    uint32_t count, uint32_t instanceOffset, glm::vec4 colour, glm::vec4 texOffset);
      size_t getAnimationIndex(Model model, std::string animationName);
      ModelAnimation* getpAnimation(Model model, int animationIndex);

  private:
      void loadQuad();
      template <class T_Vert>
      Model loadModelInfo(ModelInfo::Model& model, ModelGroup<T_Vert>& modelGroup,
			  TextureLoader* texLoader);
      ModelInfo::Model loadModelFromFile(std::string path);
      template <class T_Vert>
      void loadModelTexture(LoadedModel<T_Vert> *model, TextureLoader* texLoader);
      template <class T_Vert>
      void processLoadGroup(ModelGroup<T_Vert>* pGroup);
      template <class T_Vert>
      void stageLoadGroup(void* pMem, ModelGroup<T_Vert>* pGroup,
			  size_t &vertexDataOffset, size_t &indexDataOffset);
      void bindGroupVertexBuffer(VkCommandBuffer cmdBuff, ModelType type);
      void unloadAllModelData();
      void drawMesh(VkCommandBuffer cmdBuff,
		    ModelInGPU *modelInfo,
		    uint32_t meshIndex,
		    uint32_t instanceCount,
		    uint32_t instanceOffset);

#ifndef NO_ASSIMP
      ModelLoader modelLoader;
#endif
      DeviceState base;
      VkCommandPool pool;
      ModelGroup<Vertex2D> loaded2D;
      ModelGroup<Vertex3D> loaded3D;
      ModelGroup<VertexAnim3D> loadedAnim3D;
      std::vector<Texture> alreadyLoaded;
      std::map<size_t, ModelInGPU> models;
      VkBuffer buffer;
      VkDeviceMemory memory;

      uint32_t vertexDataSize = 0;
      uint32_t indexDataSize = 0;
      uint32_t currentIndex = 0;

      bool boundThisFrame = false;
      ModelType prevBoundType;

      size_t quadID;
  };

}


#endif
