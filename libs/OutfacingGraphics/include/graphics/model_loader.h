#ifndef OUTFACING_MODEL_LOADER
#define OUTFACING_MODEL_LOADER

#include "resources.h"
#include "model/info.h"

class ModelLoader {
 public:
    virtual Resource::Model load(
	    Resource::ModelType type,
	    std::string path,
	    std::vector<Resource::ModelAnimation>* pAnimations) = 0;
    Resource::Model load(Resource::ModelType type, std::string path) {
	return load(type, path, nullptr);
    }
    Resource::Model load(std::string path) {
	return load(Resource::ModelType::m3D, path);
    }
    virtual Resource::Model load(
	    Resource::ModelType type,
	    ModelInfo::Model &modelData,
	    std::vector<Resource::ModelAnimation>* pAnimations) = 0;
    Resource::Model load(Resource::ModelType type, ModelInfo::Model &modelData) {
	return load(type, modelData, nullptr);
    }
    Resource::Model load(ModelInfo::Model &modelData) {
	return load(Resource::ModelType::m3D, modelData);
    }
    virtual ModelInfo::Model loadModelData(std::string path) = 0;
    virtual Resource::ModelAnimation getAnimation(Resource::Model model, std::string animation) = 0;
    virtual Resource::ModelAnimation getAnimation(Resource::Model model, int index) = 0;
};

#endif /* OUTFACING_MODEL_LOADER */
