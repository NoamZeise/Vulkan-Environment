#ifndef VK_ENV_RESOURCE_POOL_H
#define VK_ENV_RESOURCE_POOL_H

#include "texture_loader.h"
#include "model_loader.h"
#include <resource_loader/font_loader.h>

struct ResourcePool {
    ResourcePool(uint32_t ID, DeviceState base, VkCommandPool pool,
		 VkCommandBuffer cmdbuff, RenderConfig config);
    ~ResourcePool();

    void loadPoolToGPU();
    void unloadStaged();
    void unloadGPU();

    void setUseGPUResources(bool value);

    TexLoaderVk* texLoader;
    ModelLoaderVk* modelLoader;
    InternalFontLoader* fontLoader;
    Resource::Pool poolID;

    bool UseGPUResources = false;
    bool usingGPUResources = false;
};


#endif
