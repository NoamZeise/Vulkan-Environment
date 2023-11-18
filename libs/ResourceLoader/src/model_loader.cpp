#include <resource_loader/model_loader.h>
#include <graphics/logger.h>
#include "assimp_loader.h"

InternalModelLoader::InternalModelLoader(Resource::Pool pool, InternalTexLoader* texLoader) {
    this->texLoader = texLoader;
    this->pool = pool;
    loader = new AssimpLoader();
}

InternalModelLoader::~InternalModelLoader() {
    clearStaged();
    clearGPU();
    delete loader;
}

void InternalModelLoader::clearStaged() {
    stage2D.clearData();
    stage3D.clearData();
    stageAnim3D.clearData();
    currentIndex = 0;
}

Resource::Model InternalModelLoader::LoadModel(
	Resource::ModelType type, std::string path,
	std::vector<Resource::ModelAnimation> *pAnimations) {
    ModelInfo::Model model = loader->LoadModel(path);
    return LoadModel(type, model, pAnimations);
}

Resource::Model InternalModelLoader::LoadModel(
	Resource::ModelType type, ModelInfo::Model &modelData,
	std::vector<Resource::ModelAnimation>* pAnimations) {
    switch(type) {
    case Resource::ModelType::m2D:
	return load(modelData, stage2D, pAnimations);
    case Resource::ModelType::m3D:
	return load(modelData, stage3D, pAnimations);
    case Resource::ModelType::m3D_Anim:
	return load(modelData, stageAnim3D, pAnimations);
    default:
	throw std::runtime_error("Model Type Not implemented in "
				 "InternalModelLoader");
    }
}

template <class T_Vert>
Resource::Model InternalModelLoader::load(ModelInfo::Model& model,
					  ModelGroup<T_Vert>& modelGroup,
					  std::vector<Resource::ModelAnimation> *pAnimations) {
    Resource::Model usermodel(currentIndex++, getModelType(T_Vert()), pool);
    modelGroup.loadModel(model, usermodel.ID);
    LoadedModel<T_Vert>* loaded = modelGroup.getPreviousModel();

    for(Mesh<T_Vert> *mesh: loaded->meshes) {
	if(mesh->texToLoad != "")
	    mesh->texture = texLoader->LoadTexture(MODEL_TEXTURE_LOCATION
						   + mesh->texToLoad);
	else
	    mesh->texture.ID = UINT32_MAX;
    }
    
    for(ModelInfo::Animation &anim: model.animations) {
	loaded->animations.push_back(Resource::ModelAnimation(model.bones, anim));
	if(pAnimations != nullptr)
	    pAnimations->push_back(loaded->animations[loaded->animations.size() - 1]);
    }
    
    LOG("Model Loaded " <<
	" - pool: " << pool.ID <<
	" - id: " << usermodel.ID);
    return usermodel;
}

void InternalModelLoader::loadQuad() {
    ModelInfo::Model q = makeQuadModel();
    quad = LoadModel(Resource::ModelType::m2D, q, nullptr);
}
