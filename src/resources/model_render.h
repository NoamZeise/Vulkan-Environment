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
      Model loadModel(std::string path, TextureLoader* texLoader);
      Model loadModel(std::string path, TextureLoader* texLoader,
		      std::vector<Resource::ModelAnimation> *pGetAnimations);
      Model load2DModel(std::vector<Vertex2D> vertices,
			std::vector<uint32_t> indices);
      void endLoading(VkCommandBuffer transferBuff);

      void bindBuffers(VkCommandBuffer cmdBuff);
      void drawModel(VkCommandBuffer cmdBuff, VkPipelineLayout layout, Model model,
		     size_t count, size_t instanceOffset);
      void drawQuad(VkCommandBuffer cmdBuff, VkPipelineLayout layout, unsigned int texID,
		    size_t count, size_t instanceOffset, glm::vec4 colour, glm::vec4 texOffset);

      size_t getAnimationIndex(Model model, std::string animationName);
      ModelAnimation* getpAnimation(Model model, int animationIndex);

  private:
      void loadQuad();

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

      size_t vertexDataSize = 0;
      size_t indexDataSize = 0;
      uint32_t currentIndex = 0;

      bool boundThisFrame = false;
      ModelType prevBoundType;

      size_t quadID;
  };

}


#endif
