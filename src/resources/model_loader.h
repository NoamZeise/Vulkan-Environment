#ifndef MODEL_RENDER_H
#define MODEL_RENDER_H

#include <map>

#include <resource_loader/model_loader.h>

#include "../device_state.h"

struct ModelInGPU;

class ModelLoaderVk : public InternalModelLoader {
public:
    ModelLoaderVk(DeviceState base, VkCommandPool cmdpool, VkCommandBuffer generalCmdBuff,
		  Resource::Pool pool, BasePoolManager *pools);
    ~ModelLoaderVk();
    void loadGPU() override;
    void clearGPU() override;

    void bindBuffers(VkCommandBuffer cmdBuff);
    void drawModel(VkCommandBuffer cmdBuff, VkPipelineLayout layout, Resource::Model model,
		   uint32_t count, uint32_t instanceOffset);
    void drawQuad(VkCommandBuffer cmdBuff, VkPipelineLayout layout, unsigned int texID,
		  uint32_t count, uint32_t instanceOffset, glm::vec4 colour, glm::vec4 texOffset);
    Resource::ModelAnimation getAnimation(Resource::Model model,
					  std::string animationName) override;
    Resource::ModelAnimation getAnimation(Resource::Model model,
					  int index) override;

private:
    template <class T_Vert>
    void processLoadGroup(ModelGroup<T_Vert>* pGroup);
    template <class T_Vert>
    void stageLoadGroup(void* pMem, ModelGroup<T_Vert>* pGroup,
			size_t &vertexDataOffset, size_t &indexDataOffset);
    void bindGroupVertexBuffer(VkCommandBuffer cmdBuff, Resource::ModelType type);
    void drawMesh(VkCommandBuffer cmdBuff,
		  ModelInGPU *modelInfo,
		  uint32_t meshIndex,
		  uint32_t instanceCount,
		  uint32_t instanceOffset);

    DeviceState base;
    VkCommandPool cmdpool;
    VkCommandBuffer cmdbuff;
    VkFence loadedFence;
    std::vector<ModelInGPU*> models;
    size_t modelTypeOffset[(size_t)Resource::ModelType::m3D_Anim + 1];
    VkBuffer buffer;
    VkDeviceMemory memory;

    uint32_t vertexDataSize = 0;
    uint32_t indexDataSize = 0;

    bool boundThisFrame = false;
    Resource::ModelType prevBoundType;
};



#endif
