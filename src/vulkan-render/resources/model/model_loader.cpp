#include "model_loader.h"
#include "assimp/anim.h"
#include "assimp/material.h"
#include "assimp/matrix4x4.h"
#include "assimp/scene.h"
#include "assimp/types.h"
#include "glm/fwd.hpp"

namespace Resource
{

ModelLoader::ModelLoader()
{

}

ModelInfo::Model ModelLoader::LoadModel(std::string path)
{
    auto model = ModelInfo::Model{};

    const aiScene *scene = importer.ReadFile(path, IMPORT_PROPS);

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		throw std::runtime_error("failed to load model at \"" + path + "\" assimp error: " + importer.GetErrorString());

	processNode(&model, scene->mRootNode, scene, aiMatrix4x4(), -1);

#ifndef  NDEBUG
    std::cout << "model bone count: " << model.bones.size() << std::endl;
    std::cout << "model animation count: " << scene->mNumAnimations << std::endl;
#endif

    if(scene->HasAnimations())
    {
        model.animatedModel = true;
        for(size_t i = 0; i < scene->mNumAnimations; i++)
        {
        #ifndef NDEBUG
            std::cout << "loading animation: " << scene->mAnimations[i]->mName.C_Str() << std::endl;
        #endif
            buildAnimation(&model, scene->mAnimations[i]);
        }
    }

    importer.FreeScene();

    return model;            
}

void ModelLoader::processNode(ModelInfo::Model* model, aiNode* node, const aiScene* scene, aiMatrix4x4 parentTransform, int parentNode)
{
    //std::cout << "processing node: " << node->mName.C_Str() << std::endl;
	aiMatrix4x4 transform = parentTransform * node->mTransformation;

    model->nodes.push_back(ModelInfo::Node{});
    model->nodes.back().parentNode = parentNode;
    model->nodes.back().transform = aiToGLM(node->mTransformation);
    int thisID = static_cast<int>(model->nodes.size() - 1);
    model->nodeMap[node->mName.C_Str()] = thisID;
    if(parentNode >= 0)
    {
        model->nodes[parentNode].children.push_back(thisID);
    }
	for(unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
		model->meshes.push_back(ModelInfo::Mesh());
		processMesh(model, mesh, scene, transform);
	}
	for(unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(model, node->mChildren[i], scene, transform, thisID);
	}
}
void ModelLoader::processMesh(ModelInfo::Model* model, aiMesh* aimesh, const aiScene* scene, aiMatrix4x4 transform)
{

    ModelInfo::Mesh* mesh = &model->meshes[model->meshes.size() - 1];
    mesh->bindTransform = aiToGLM(transform);

    auto material = scene->mMaterials[aimesh->mMaterialIndex];

    aiColor3D diffuseColour;
    if(material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColour) != AI_SUCCESS)
    {
        std::cout << "warning: failed to get diffuse colour\n";
        mesh->diffuseColour = glm::vec4(1);
    }
    else
    {
        mesh->diffuseColour = glm::vec4(diffuseColour.r, diffuseColour.g, diffuseColour.b, 1);
    }

    for(unsigned int i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); i++)
    {
        aiString texPath;
        material->GetTexture(aiTextureType_DIFFUSE, i, &texPath);
        mesh->diffuseTextures.push_back(texPath.C_Str());
        for(size_t i = 0; i < mesh->diffuseTextures.back().size(); i++)
            if(mesh->diffuseTextures.back().at(i) == '\\')
                mesh->diffuseTextures[mesh->diffuseTextures.size() - 1][i] = '/';
    }

	//vertcies
	for(unsigned int i = 0; i < aimesh->mNumVertices;i++)
	{
		ModelInfo::Vertex vertex;
		vertex.Position.x = aimesh->mVertices[i].x;
		vertex.Position.y = aimesh->mVertices[i].y;
		vertex.Position.z = aimesh->mVertices[i].z;
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
    for(unsigned int i = 0; i < aimesh->mNumBones; i++)
    {
        auto aibone = aimesh->mBones[i];
        unsigned int boneID;
        std::string boneName = aibone->mName.C_Str();
        if(model->boneMap.find(boneName) == model->boneMap.end())
        {
            model->bones.push_back(aiToGLM(aibone->mOffsetMatrix));
            boneID = static_cast<unsigned int>(model->bones.size() - 1);
            model->boneMap[boneName] = boneID;
        }
        else
            boneID = model->boneMap[boneName];


        for(unsigned int bone = 0; bone < aibone->mNumWeights; bone++)
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

void ModelLoader::buildAnimation(ModelInfo::Model* model, aiAnimation* aiAnim)
{
    model->animations.push_back(ModelInfo::Animation());
    model->animationMap[aiAnim->mName.C_Str()] = static_cast<unsigned int>(model->animations.size());
    auto anim = &model->animations[model->animations.size() - 1];
    anim->name = aiAnim->mName.C_Str();
    //copy nodes from model
    anim->nodes.resize(model->nodes.size());
    for(int i = 0; i < model->nodes.size(); i++)
        anim->nodes[i].modelNode = model->nodes[i];

    anim->duration = aiAnim->mDuration;
    anim->ticks = aiAnim->mTicksPerSecond * 0.001; //for ms time
    if(anim->ticks == 0)
    {
    #ifndef NDEBUG
        std::cout << "WARNING: format does not specify ticks, using 100/s\n";
    #endif
        anim->ticks = 100;
    }

    //set animation props for anim nodes
    for(unsigned int i = 0 ; i < aiAnim->mNumChannels; i++)
    {
        auto channel = aiAnim->mChannels[i];
        std::string nodeName = channel->mNodeName.C_Str();
        auto node = &anim->nodes[model->nodeMap[nodeName]];
        //std::cout <<  "node animation process: " << nodeName << std::endl;
        //std::cout << "keys: " << channel->mNumPositionKeys << std::endl;
        if(model->boneMap.find(nodeName) != model->boneMap.end())
        {
            node->boneID = model->boneMap[nodeName];
            node->boneOffset = model->bones[node->boneID];
        }
        //else bone directly affects no nodes;

        for(unsigned int pos = 0; pos < channel->mNumPositionKeys; pos++)
        {
            auto posKey = channel->mPositionKeys[pos];
            node->positions.push_back(ModelInfo::AnimationKey::Position{});
            node->positions.back().Pos = glm::vec3(posKey.mValue.x, posKey.mValue.y, posKey.mValue.z);
            node->positions.back().time = posKey.mTime;
        }

        for(unsigned int rot = 0; rot < channel->mNumRotationKeys; rot++)
        {
            auto rotKey = channel->mRotationKeys[rot];
            node->rotationsQ.push_back(ModelInfo::AnimationKey::RotationQ{});
            node->rotationsQ.back().Rot = glm::quat(rotKey.mValue.w, rotKey.mValue.x, rotKey.mValue.y, rotKey.mValue.z);
            node->rotationsQ.back().time = rotKey.mTime;
        }

        for(unsigned int scl = 0; scl < channel->mNumScalingKeys; scl++)
        {
            auto scaleKey = channel->mScalingKeys[scl];
            node->scalings.push_back(ModelInfo::AnimationKey::Scaling{});
            node->scalings.back().scale = glm::vec3(scaleKey.mValue.x, scaleKey.mValue.y, scaleKey.mValue.z);
            node->scalings.back().time = scaleKey.mTime;
        }
    }
}

}
