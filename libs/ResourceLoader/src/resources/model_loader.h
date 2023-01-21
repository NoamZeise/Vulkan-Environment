#ifndef GL_MODEL_LOADER_H
#define GL_MODEL_LOADER_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <resources/model/model_info.h>

namespace Resource
{


class ModelLoader
{
public:
    ModelLoader();

    ModelInfo::Model LoadModel(std::string path);

private:

    Assimp::Importer importer;

    void processNode(ModelInfo::Model* model, aiNode* node, const aiScene* scene, aiMatrix4x4 parentTransform, int parentNode);
    void processMesh(ModelInfo::Model* model, aiMesh* aimesh, const aiScene* scene, aiMatrix4x4 transform);
    void buildAnimation(ModelInfo::Model* model, aiAnimation* aiAnim);
};
}

#endif
