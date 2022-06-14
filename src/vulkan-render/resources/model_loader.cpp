#include "model_loader.h"
#include "assimp/material.h"
#include "assimp/scene.h"
#include "glm/fwd.hpp"

namespace Resource
{

ModelLoader::ModelLoader()
{

}

ModelInfo::Model ModelLoader::LoadModel(std::string path)
{
#ifndef NDEBUG
    std::cout << "loading model: " << path << std::endl;
#endif
    auto model = ModelInfo::Model{};

    const aiScene *scene = importer.ReadFile(path, IMPORT_PROPS);

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		throw std::runtime_error("failed to load model at \"" + path + "\" assimp error: " + importer.GetErrorString());


    glm::mat4 transform = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	transform = glm::scale(transform, glm::vec3(0.02f));
	aiMatrix4x4 aiTransform = glmToAi(transform);
	processNode(&model, scene->mRootNode, scene, aiTransform);

    //TODO load animations

    return model;            
}

void ModelLoader::processNode(ModelInfo::Model* model, aiNode* node, const aiScene* scene, aiMatrix4x4 parentTransform)
{
	aiMatrix4x4 transform = parentTransform * node->mTransformation;
	for(unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
		model->meshes.push_back(ModelInfo::Mesh());
		processMesh(model, mesh, scene, transform);
	}
	for(unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(model, node->mChildren[i], scene, transform);
	}
}
void ModelLoader::processMesh(ModelInfo::Model* model, aiMesh* aimesh, const aiScene* scene, aiMatrix4x4 transform)
{
    ModelInfo::Mesh* mesh = &model->meshes[model->meshes.size() - 1];

    auto material = scene->mMaterials[aimesh->mMaterialIndex];
    for( int i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); i++)
    {
        aiString texPath;
        material->GetTexture(aiTextureType_DIFFUSE, i, &texPath);
        mesh->diffuseTextures.push_back(texPath.C_Str());
    }

	//vertcies
	for(unsigned int i = 0; i < aimesh->mNumVertices;i++)
	{
		aiVector3D transformedVertex = transform * aimesh->mVertices[i];
		ModelInfo::Vertex vertex;
		vertex.Position.x = transformedVertex.x;
		vertex.Position.y = transformedVertex.y;
		vertex.Position.z = transformedVertex.z;
		if(aimesh->HasNormals())
		{
			vertex.Normal.x = aimesh->mNormals[i].x;
			vertex.Normal.y = aimesh->mNormals[i].y;
			vertex.Normal.z = aimesh->mNormals[i].z;
		}
		else
			vertex.Normal = glm::vec3(0);
		if(aimesh->mTextureCoords[0])
		{
			vertex.TexCoord.x = aimesh->mTextureCoords[0][i].x;
			vertex.TexCoord.y = aimesh->mTextureCoords[0][i].y;
		}
		else
			vertex.TexCoord = glm::vec2(0);

		mesh->verticies.push_back(vertex);
	}

    //bones - relies on verticies
    for(int i = 0; i < aimesh->mNumBones; i++)
    {
        auto aibone = aimesh->mBones[i];
        unsigned int boneID;
        std::string boneName = aibone->mName.C_Str();
        if(model->boneMap.find(boneName) == model->boneMap.end())
        {
            model->bones.push_back(aiToGLM(aibone->mOffsetMatrix));
            boneID = model->bones.size() - 1;
            model->boneMap[boneName] = boneID;
        }
        else
            boneID = model->boneMap[boneName];


        for(int bone = 0; bone < aibone->mNumWeights; bone++)
        {
            auto vertexWeight = aibone->mWeights[bone];
            mesh->verticies[vertexWeight.mVertexId].BoneIDs.push_back(boneID);
            mesh->verticies[vertexWeight.mVertexId].BoneWeights.push_back(vertexWeight.mWeight);
        }
    }

	//indicies
	for(unsigned int i = 0; i < aimesh->mNumFaces; i++)
	{
		aiFace face = aimesh->mFaces[i];
		for(unsigned int j = 0; j < face.mNumIndices; j++)
			mesh->indicies.push_back(face.mIndices[j]);
	}


}

}
