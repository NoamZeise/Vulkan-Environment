/// An importer for loading model files from disk and transforming them into
/// ModelInfo's.
/// This file makes it easy to optionally compile the library with assimp support.

#ifndef RES_LOADER_ASSIMP_H
#define RES_LOADER_ASSIMP_H

#include <graphics/model/info.h>
#include <string>

#ifdef NO_ASSIMP

class AssimpLoader {
public:
    ModelInfo::Model LoadModel(std::string path);
};

#else
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/anim.h>
#include <assimp/material.h>
#include <assimp/matrix4x4.h>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>

class AssimpLoader {
 public:
    ModelInfo::Model LoadModel(std::string path);
private:
    Assimp::Importer importer;

    void buildAnimations(ModelInfo::Model* model, aiAnimation* aiAnim);
    void processNode(ModelInfo::Model* model, aiNode* node,
		     const aiScene* scene, aiMatrix4x4 parentTransform, int parentNode);
    void processMesh(ModelInfo::Model* model, aiMesh* aimesh,
		     const aiScene* scene, aiMatrix4x4 transform);
};

#endif

#endif /* RES_LOADER_ASSIMP_H */

