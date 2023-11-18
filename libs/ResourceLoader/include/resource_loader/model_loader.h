#ifndef GL_MODEL_LOADER_H
#define GL_MODEL_LOADER_H

#include "texture_loader.h"
#include <resource_loader/vertex_model.h>
#include <graphics/model_loader.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

namespace Resource {
  
  class AssimpLoader {
  public:
      AssimpLoader();
      ModelInfo::Model LoadModel(std::string path);
  private:
      Assimp::Importer importer;
      void processNode(ModelInfo::Model* model, aiNode* node, const aiScene* scene,
		       aiMatrix4x4 parentTransform, int parentNode);
      void processMesh(ModelInfo::Model* model, aiMesh* aimesh, const aiScene* scene,
		       aiMatrix4x4 transform);
      void buildAnimation(ModelInfo::Model* model, aiAnimation* aiAnim);
  };

} // namespace Resource

#include <map>

struct GPUMesh {
    Resource::Texture texture;
    glm::vec4 diffuseColour;

    template <typename T_Vert>
    void load(Mesh<T_Vert>* data) {
	diffuseColour = data->diffuseColour;
	texture = data->texture;
    }
};

struct GPUModel {
    std::vector<Resource::ModelAnimation> animations;
    std::map<std::string, int> animationMap;
    Resource::ModelType type;

    template <typename T_Vert>
    GPUModel(LoadedModel<T_Vert> &model) {
	animations.resize(model.animations.size());
	for (int i = 0; i < model.animations.size(); i++) {
	    animations[i] = model.animations[i];
	    animationMap[model.animations[i].getName()] = i;
	}   
    }
};

class InternalModelLoader : public ModelLoader {
public:
    InternalModelLoader(Resource::Pool pool, InternalTexLoader* texLoader);
    ~InternalModelLoader();
    
    Resource::Model LoadModel(
	    Resource::ModelType type,
	    std::string path,
	    std::vector<Resource::ModelAnimation>* pAnimations) override;
    Resource::Model LoadModel(
	    Resource::ModelType type,
	    ModelInfo::Model &modelData,
	    std::vector<Resource::ModelAnimation>* pAnimations) override;

    virtual void loadGPU() = 0;
    virtual void clearGPU() {};
    void clearStaged();

    /*    Resource::ModelAnimation getAnimation(Resource::Model model, std::string animation) override;
    Resource::ModelAnimation getAnimation(Resource::Model model, int index) override;*/
protected:

    //  virtual GPUModel getModel() { return {}; }
    
    Resource::Pool pool;
    InternalTexLoader *texLoader;
    unsigned int currentIndex = 0;
    ModelGroup<Vertex2D> stage2D;
    ModelGroup<Vertex3D> stage3D;
    ModelGroup<VertexAnim3D> stageAnim3D;
    Resource::Model quad;
    Resource::AssimpLoader loader;
    template <class T_Vert>
    Resource::Model load(ModelInfo::Model& model,
			 ModelGroup<T_Vert>& modelGroup,
			 std::vector<Resource::ModelAnimation> *pAnimations);
    void loadQuad();
};



#endif
