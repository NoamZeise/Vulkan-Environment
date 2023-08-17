#include "resource_pool.h"

ResourcePool::ResourcePool(uint32_t ID, DeviceState base, VkCommandPool pool, RenderConfig config) {
    poolID = Resource::ResourcePool(ID);
    texLoader = new Resource::TextureLoader(base, pool, poolID, config);
    modelLoader = new Resource::ModelRender(base, pool, poolID);
    fontLoader = new Resource::FontLoader(poolID);
}

ResourcePool::~ResourcePool() {
    delete fontLoader;
    delete modelLoader;
    delete texLoader;
}

#include "../logger.h"

void ResourcePool::loadPoolToGPU(VkCommandBuffer generalCmdBuff) {
    texLoader->endLoading();
    fontLoader->EndLoading();
    modelLoader->endLoading(generalCmdBuff);
}

void ResourcePool::unloadStaged() {
    texLoader->UnloadStaged();
    modelLoader->unloadStaged();
    fontLoader->UnloadStaged();
}

void ResourcePool::unloadGPU() {
    texLoader->UnloadGPU();
    modelLoader->unloadGPU();
    fontLoader->UnloadFonts();
}
