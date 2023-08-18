#ifndef VK_ENV_RESOURCE_POOL_H
#define VK_ENV_RESOURCE_POOL_H

#include <graphics/resources.h>
#include <graphics/render_config.h>
#include <volk.h>

#include "texture_loader.h"
#include "model_render.h"
#include "font_loader.h"

class DeviceState;
namespace Resource {
    class TextureLoader;
    class ModelRender;
    class FontLoader;
};

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

    Resource::TextureLoader* texLoader;
    Resource::ModelRender* modelLoader;
    Resource::FontLoader* fontLoader;
    Resource::ResourcePool poolID;

    bool UseGPUResources = false;
    bool usingGPUResources = false;
};


#endif
