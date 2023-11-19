#include "resource_pool.h"

ResourcePoolVk::ResourcePoolVk(uint32_t ID, DeviceState base, VkCommandPool pool, VkCommandBuffer cmdbuff, RenderConfig config) {
    poolID = Resource::Pool(ID);
    texLoader = new TexLoaderVk(base, pool, poolID, config);
    modelLoader = new ModelLoaderVk(base, pool, cmdbuff, poolID, texLoader);
    fontLoader = new InternalFontLoader(poolID, texLoader);
}

ResourcePoolVk::~ResourcePoolVk() {
    delete fontLoader;
    delete modelLoader;
    delete texLoader;
}

void ResourcePoolVk::setUseGPUResources(bool value) {
    this->UseGPUResources = value;
}

void ResourcePoolVk::loadPoolToGPU() {
    texLoader->loadGPU();
    fontLoader->loadGPU();
    modelLoader->loadGPU();
    UseGPUResources = true;
    usingGPUResources = false;
}

void ResourcePoolVk::unloadStaged() {
    texLoader->clearStaged();
    modelLoader->clearStaged();
    fontLoader->clearStaged();
}

void ResourcePoolVk::unloadGPU() {
    texLoader->clearGPU();
    modelLoader->clearGPU();
    fontLoader->clearGPU();
    UseGPUResources = false;
    usingGPUResources = false;
}
