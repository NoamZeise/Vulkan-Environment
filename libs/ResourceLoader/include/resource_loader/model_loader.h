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

    virtual void loadGPU() {};
    virtual void clearGPU() {};
    void clearStaged();

protected:

    Resource::Pool pool;
    InternalTexLoader *texLoader;
    unsigned int currentIndex = 0;
    ModelGroup<Vertex2D> stage2D;
    ModelGroup<Vertex3D> stage3D;
    ModelGroup<VertexAnim3D> stageAnim3D;
    
    Resource::AssimpLoader loader;
    template <class T_Vert>
    Resource::Model load(ModelInfo::Model& model,
			 ModelGroup<T_Vert>& modelGroup,
			 std::vector<Resource::ModelAnimation> *pAnimations);
};



#endif
