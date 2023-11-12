#ifndef OUTFACING_MODEL_LOADER
#define OUTFACING_MODEL_LOADER

#include "resources.h"
#include "model/info.h"

class ModelLoader {
 public:
    virtual Resource::Model LoadModel(
	    Resource::ModelType type,
	    std::string path,
	    std::vector<Resource::ModelAnimation>* pAnimations) = 0;
    virtual Resource::Model LoadModel(
	    Resource::ModelType type,
	    ModelInfo::Model &modelData,
	    std::vector<Resource::ModelAnimation>* pAnimations) = 0;
    virtual Resource::ModelAnimation getAnimation(Resource::Model model,
						  std::string animation) = 0;
    virtual Resource::ModelAnimation getAnimation(Resource::Model model,
						  int index) = 0;
};

#endif /* OUTFACING_MODEL_LOADER */
