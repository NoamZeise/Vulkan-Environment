#ifndef VK_ENV_RESOURCE_POOL_H
#define VK_ENV_RESOURCE_POOL_H

#include "texture_loader.h"
#include "model_loader.h"
#include <resource_loader/font_loader.h>
#include <graphics/resource_pool.h>

class ResourcePoolVk : public ResourcePool {
 public:
    ResourcePoolVk(uint32_t ID, DeviceState base, VkCommandPool pool,
		 VkCommandBuffer cmdbuff, RenderConfig config);
    ~ResourcePoolVk();

    void loadPoolToGPU();
    void unloadStaged();
    void unloadGPU();

    ModelLoader* model() override { return modelLoader; }
    TextureLoader* tex() override { return texLoader; }
    FontLoader* font()   override { return fontLoader; }
    

    void setUseGPUResources(bool value);

    // private:

    TexLoaderVk* texLoader;
    ModelLoaderVk* modelLoader;
    InternalFontLoader* fontLoader;
    Resource::Pool poolID;

    bool UseGPUResources = false;
    bool usingGPUResources = false;
};


#endif
