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

Resource::Model ResourcePool::loadModel(Resource::ModelType type, std::string path, std::vector<Resource::ModelAnimation> *pGetAnimations) {
    return modelLoader->loadModel(type, path, texLoader, pGetAnimations);
}

Resource::Model ResourcePool::loadModel(Resource::ModelType type, ModelInfo::Model &model,
					std::vector<Resource::ModelAnimation> *pGetAnimations) {
    return modelLoader->loadModel(type, model, texLoader, pGetAnimations);
}

Resource::Font ResourcePool::LoadFont(std::string file) {
    return fontLoader->LoadFont(file, texLoader);
}

void ResourcePool::loadPoolToGPU(VkCommandBuffer generalCmdBuff) {
    texLoader->endLoading();
    fontLoader->EndLoading();
    modelLoader->endLoading(generalCmdBuff);
    UseGPUResources = true;
    usingGPUResources = false;
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
    UseGPUResources = false;
    usingGPUResources = false;
}
