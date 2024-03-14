#ifndef OUTFACING_MODEL_LOADER
#define OUTFACING_MODEL_LOADER

#include "resources.h"
#include "model/info.h"


class ModelLoader {
    const std::string DEFAULT_TEXTURE_PATH = "textures/";
 public:
    virtual Resource::Model load(
	    Resource::ModelType type,
	    ModelInfo::Model &modelData,
	    std::string textureFolder,
	    std::vector<Resource::ModelAnimation>* pAnimations) = 0;
    
    Resource::Model load(Resource::ModelType type, ModelInfo::Model &modelData) {
	return load(type, modelData, DEFAULT_TEXTURE_PATH, nullptr);
    }
    
    Resource::Model load(ModelInfo::Model &modelData) {
	return load(Resource::ModelType::m3D, modelData);
    }
    
    Resource::Model load(
	    Resource::ModelType type,
	    std::string path,
	    std::string textureFolder,
	    std::vector<Resource::ModelAnimation>* pAnimations) {
	ModelInfo::Model model = loadModelData(path);
	return load(type, model, textureFolder, pAnimations);
    }
    
    Resource::Model load(
	    Resource::ModelType type,
	    std::string path,
	    std::vector<Resource::ModelAnimation>* pAnimations) {
	return load(type, path, DEFAULT_TEXTURE_PATH, pAnimations);
    }
    
    Resource::Model load(Resource::ModelType type, std::string path, std::string textureFolder) {
	return load(type, path, textureFolder, nullptr);
    }
    
    Resource::Model load(Resource::ModelType type, std::string path) {
	return load(type, path, DEFAULT_TEXTURE_PATH, nullptr);
    }
    
    Resource::Model load(std::string path) {
	return load(Resource::ModelType::m3D, path);
    }

    
    virtual ModelInfo::Model loadModelData(std::string path) = 0;

    
    virtual Resource::ModelAnimation getAnimation(Resource::Model model, std::string animation) = 0;


    virtual Resource::ModelAnimation getAnimation(Resource::Model model, int index) = 0;
};

#endif /* OUTFACING_MODEL_LOADER */
