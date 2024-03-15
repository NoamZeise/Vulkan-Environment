#include "resource_pool.h"

ResourcePoolVk::ResourcePoolVk(uint32_t poolID, BasePoolManager* pools, DeviceState base, VkCommandPool cmdpool, VkCommandBuffer cmdbuff, RenderConfig config) {
    this->pool = Resource::Pool(poolID);
    texLoader = new TexLoaderVk(base, cmdpool, pool, config);
    modelLoader = new ModelLoaderVk(base, cmdpool, cmdbuff, pool, pools);
    fontLoader = new InternalFontLoader(pool, texLoader);
}

ResourcePoolVk::~ResourcePoolVk() {
    delete fontLoader;
    delete modelLoader;
    delete texLoader;
}

void ResourcePoolVk::setUseGPUResources(bool value) {
    this->UseGPUResources = value;
}

void ResourcePoolVk::loadGpu() {
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
