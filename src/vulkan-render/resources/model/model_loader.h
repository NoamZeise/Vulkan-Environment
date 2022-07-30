#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <assimp/anim.h>
#include <assimp/matrix4x4.h>
#include <assimp/mesh.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <stdexcept>


#include "model_info.h"

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

namespace
{
    const auto IMPORT_PROPS =
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenNormals |
        aiProcess_LimitBoneWeights;


    inline glm::mat4 aiToGLM(aiMatrix4x4 mat)
    {
        glm::mat4 glmmat;
        glmmat[0][0] = mat.a1;
        glmmat[0][1] = mat.b1;
        glmmat[0][2] = mat.c1;
        glmmat[0][3] = mat.d1;

        glmmat[1][0] = mat.a2;
        glmmat[1][1] = mat.b2;
        glmmat[1][2] = mat.c2;
        glmmat[1][3] = mat.d2;

        glmmat[2][0] = mat.a3;
        glmmat[2][1] = mat.b3;
        glmmat[2][2] = mat.c3;
        glmmat[2][3] = mat.d3;

        glmmat[3][0] = mat.a4;
        glmmat[3][1] = mat.b4;
        glmmat[3][2] = mat.c4;
        glmmat[3][3] = mat.d4;

        return glmmat;
    }

    inline aiMatrix4x4 glmToAi(glm::mat4 mat)
    {
        aiMatrix4x4 aiMat = aiMatrix4x4(
		mat[0][0], mat[0][1], mat[0][2], mat[0][3],
		mat[1][0], mat[1][1], mat[1][2], mat[1][3],
		mat[2][0], mat[2][1], mat[2][2], mat[2][3],
		mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
        return aiMat;
    }
}

}

#endif
