#ifndef VK_ENV_RESOURCE_POOL_H
#define VK_ENV_RESOURCE_POOL_H

#include "texture_loader.h"
#include "model_loader.h"
#include <resource_loader/font_loader.h>
#include <resource_loader/pool_manager.h>
#include <graphics/resource_pool.h>

class ResourcePoolVk : public ResourcePool {
 public:
    ResourcePoolVk(uint32_t poolID, BasePoolManager* pools, DeviceState base, VkCommandPool cmdpool,
		 VkCommandBuffer cmdbuff, RenderConfig config);
    ~ResourcePoolVk();

    void loadGpu();
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

    bool UseGPUResources = false;
    bool usingGPUResources = false;
};

MAKE_POOL_MANAGER(PoolManagerVk, ResourcePoolVk)

#endif
