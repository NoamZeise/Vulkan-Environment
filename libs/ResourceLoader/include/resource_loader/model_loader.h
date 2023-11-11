#ifndef GL_MODEL_LOADER_H
#define GL_MODEL_LOADER_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <graphics/model_loader.h>
#include <graphics/texture_loader.h>

namespace Resource {
  
class AssimpLoader {
public:
    AssimpLoader();
    ModelInfo::Model LoadModel(std::string path);
private:
    Assimp::Importer importer;
    void processNode(ModelInfo::Model* model, aiNode* node, const aiScene* scene, aiMatrix4x4 parentTransform, int parentNode);
    void processMesh(ModelInfo::Model* model, aiMesh* aimesh, const aiScene* scene, aiMatrix4x4 transform);
    void buildAnimation(ModelInfo::Model* model, aiAnimation* aiAnim);
};

} // namespace Resource

class InternalModelLoader : public ModelLoader {
public:
    InternalModelLoader(Resource::ResourcePool pool, TextureLoader* texLoader);
    
    Resource::Model LoadModel(
	    Resource::ModelType type,
	    std::string path,
	    std::vector<Resource::ModelAnimation>* pAnimations) override;
    
    Resource::Model LoadModel(
	    Resource::ModelType type,
	    ModelInfo::Model &modelData,
	    std::vector<Resource::ModelAnimation>* pAnimations) override;

    virtual void loadGPU() = 0;
    virtual void clearGPU() = 0;
    void clearStaged();

    
protected:
    
};

#endif
