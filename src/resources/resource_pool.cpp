#include "resource_pool.h"

ResourcePool::ResourcePool(uint32_t ID, DeviceState base, VkCommandPool pool, RenderConfig config) {
    poolID = Resource::ResourcePool(ID);
    texLoader = new TexLoaderVk(base, pool, poolID, config);
    modelLoader = new Resource::ModelRender(base, pool, poolID);
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

Resource::Model ResourcePool::loadModel(Resource::ModelType type, std::string path, std::vector<Resource::ModelAnimation> *pGetAnimations) {
    return modelLoader->loadModel(type, path, texLoader, pGetAnimations);
}

Resource::Model ResourcePool::loadModel(Resource::ModelType type, ModelInfo::Model &model,
					std::vector<Resource::ModelAnimation> *pGetAnimations) {
    return modelLoader->loadModel(type, model, texLoader, pGetAnimations);
}

Resource::Font ResourcePool::LoadFont(std::string file) {
    return fontLoader->LoadFont(file);
}

void ResourcePool::loadPoolToGPU(VkCommandBuffer generalCmdBuff) {
    texLoader->loadGPU();
    fontLoader->loadGPU();
    modelLoader->endLoading(generalCmdBuff);
    UseGPUResources = true;
    usingGPUResources = false;
}

void ResourcePool::unloadStaged() {
    texLoader->clearStaged();
    modelLoader->unloadStaged();
    fontLoader->clearStaged();
}

void ResourcePool::unloadGPU() {
    texLoader->clearGPU();
    modelLoader->unloadGPU();
    fontLoader->clearGPU();
    UseGPUResources = false;
    usingGPUResources = false;
}
