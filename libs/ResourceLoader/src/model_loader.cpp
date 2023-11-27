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
    delete loader;
}

void InternalModelLoader::clearStaged() {
    stage2D.clearData();
    stage3D.clearData();
    stageAnim3D.clearData();
    currentIndex = 0;
}

Resource::Model InternalModelLoader::load(
	Resource::ModelType type, std::string path,
	std::vector<Resource::ModelAnimation> *pAnimations) {
    ModelInfo::Model model = loader->LoadModel(path);
    return load(type, model, pAnimations);
}

Resource::Model InternalModelLoader::load(
	Resource::ModelType type, ModelInfo::Model &modelData,
	std::vector<Resource::ModelAnimation>* pAnimations) {
    switch(type) {
    case Resource::ModelType::m2D:
	return loadData(modelData, stage2D, pAnimations);
    case Resource::ModelType::m3D:
	return loadData(modelData, stage3D, pAnimations);
    case Resource::ModelType::m3D_Anim:
	return loadData(modelData, stageAnim3D, pAnimations);
    default:
	throw std::runtime_error("Model Type Not implemented in "
				 "InternalModelLoader");
    }
}

template <class T_Vert>
Resource::Model InternalModelLoader::loadData(ModelInfo::Model& model,
					  ModelGroup<T_Vert>& modelGroup,
					  std::vector<Resource::ModelAnimation> *pAnimations) {
    Resource::Model usermodel(currentIndex++, getModelType(T_Vert()), pool);
    modelGroup.loadModel(model, usermodel.ID);
    LoadedModel<T_Vert>* loaded = modelGroup.getPreviousModel();

    for(Mesh<T_Vert> *mesh: loaded->meshes) {
	if(mesh->texToLoad != "")
	    mesh->texture = texLoader->load(MODEL_TEXTURE_LOCATION
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
    quad = load(Resource::ModelType::m2D, q, nullptr);
}
