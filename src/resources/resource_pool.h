#ifndef VK_ENV_RESOURCE_POOL_H
#define VK_ENV_RESOURCE_POOL_H

#include "texture_loader.h"
#include "model_render.h"
#include "font_loader.h"

struct ResourcePool {
    ResourcePool(uint32_t ID, DeviceState base, VkCommandPool pool, RenderConfig config);
    ~ResourcePool();

    void loadPoolToGPU(VkCommandBuffer generalCmdBuff);
    void unloadStaged();
    void unloadGPU();

    void setUseGPUResources(bool value);

    //injects TextureLoader ref into calls to model+font loaders
    Resource::Model loadModel(Resource::ModelType type, std::string path, std::vector<Resource::ModelAnimation> *pGetAnimations);
    Resource::Model loadModel(Resource::ModelType type, ModelInfo::Model& model, std::vector<Resource::ModelAnimation> *pGetAnimations);
    Resource::Font LoadFont(std::string file);

    TexLoaderVk* texLoader;
    Resource::ModelRender* modelLoader;
    Resource::FontLoader* fontLoader;
    Resource::ResourcePool poolID;

    bool UseGPUResources = false;
    bool usingGPUResources = false;
};


#endif
