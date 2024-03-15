#include <resource_loader/model_loader.h>
#include <graphics/logger.h>
#include "assimp_loader.h"

InternalModelLoader::InternalModelLoader(Resource::Pool pool, BasePoolManager* pools) {
    this->pool = pool;
    this->pools = pools;
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
	Resource::ModelType type, ModelInfo::Model &modelData,
	std::string textureFolder,
	std::vector<Resource::ModelAnimation>* pAnimations) {
    switch(type) {
    case Resource::ModelType::m2D:
	return loadData(modelData, stage2D, textureFolder, pAnimations);
    case Resource::ModelType::m3D:
	return loadData(modelData, stage3D, textureFolder, pAnimations);
    case Resource::ModelType::m3D_Anim:
	return loadData(modelData, stageAnim3D, textureFolder, pAnimations);
    default:
	throw std::runtime_error("Model Type Not implemented in "
				 "InternalModelLoader");
    }
}

ModelInfo::Model InternalModelLoader::loadModelData(std::string path) {
    return loader->LoadModel(path);
}

template <class T_Vert>
Resource::Model InternalModelLoader::loadData(ModelInfo::Model& model,
					      ModelGroup<T_Vert>& modelGroup,
					      std::string textureFolder,
					      std::vector<Resource::ModelAnimation> *pAnimations) {
    Resource::Model usermodel(currentIndex++, getModelType(T_Vert()), pool);
    modelGroup.loadModel(model, usermodel.ID);
    LoadedModel<T_Vert>* loaded = modelGroup.getPreviousModel();
    if(textureFolder.size() > 0 && textureFolder[textureFolder.size() - 1] != '/') {
	textureFolder.push_back('/');
    }
    for(Mesh<T_Vert> *mesh: loaded->meshes) {
	if(mesh->texToLoad != "")
	    mesh->texture = pools->tex(pool)->load(textureFolder + mesh->texToLoad);
	else
	    mesh->texture.ID = Resource::NULL_ID;
    }
    
    for(ModelInfo::Animation &anim: model.animations) {
	if(model.bones.size() >= Resource::MAX_BONES)
	    LOG_CERR("Model had more bones than MAX_BONES, "
		     "consider upping the shader max bones", "Warning: ");
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
    quad = load(Resource::ModelType::m2D, q, "", nullptr);
}

int modelGetTexID(Resource::Model model, Resource::Texture texture, BasePoolManager* pools) {
    Resource::Texture meshTex = model.overrideTexture.ID == Resource::NULL_ID ?
	texture : model.overrideTexture;    
    return meshTex.ID != Resource::NULL_ID ?
	pools->tex(meshTex)->getViewIndex(meshTex) : -1;
}
