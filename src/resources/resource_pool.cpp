#include "resource_pool.h"

ResourcePool::ResourcePool(uint32_t ID, DeviceState base, VkCommandPool pool, RenderConfig config) {
    poolID = Resource::Pool(ID);
    texLoader = new TexLoaderVk(base, pool, poolID, config);
    modelLoader = new ModelLoaderVk(base, pool, poolID, texLoader);
    fontLoader = new InternalFontLoader(poolID, texLoader);
}

ResourcePool::~ResourcePool() {
    delete fontLoader;
    delete modelLoader;
    delete texLoader;
}

void ResourcePool::setUseGPUResources(bool value) {
    this->UseGPUResources = value;
}

void ResourcePool::loadPoolToGPU(VkCommandBuffer generalCmdBuff) {
    texLoader->loadGPU();
    fontLoader->loadGPU();
    modelLoader->loadGPU(generalCmdBuff);
    UseGPUResources = true;
    usingGPUResources = false;
}

void ResourcePool::unloadStaged() {
    texLoader->clearStaged();
    modelLoader->clearStaged();
    fontLoader->clearStaged();
}

void ResourcePool::unloadGPU() {
    texLoader->clearGPU();
    modelLoader->clearGPU();
    fontLoader->clearGPU();
    UseGPUResources = false;
    usingGPUResources = false;
}
