#include "assimp_loader.h"

#include <stdexcept>
#include <graphics/logger.h>

#ifdef NO_ASSIMP
ModelInfo::Model AssimpLoader::LoadModel(std::string path) {
    throw std::runtime_error("FATAL_ERROR: Tried to load a model "
			     "using a path, but library was compiled "
			     "with NO_ASSIMP defined!");
}
#else 

const auto IMPORT_PROPS =
    aiProcess_CalcTangentSpace |
    aiProcess_Triangulate |
    aiProcess_FlipUVs |
    aiProcess_JoinIdenticalVertices |
    aiProcess_GenNormals |
    aiProcess_LimitBoneWeights;
inline glm::mat4 aiToGLM(aiMatrix4x4 mat);
inline aiMatrix4x4 glmToAi(glm::mat4 mat);

ModelInfo::Model AssimpLoader::LoadModel(std::string path) {
    auto model = ModelInfo::Model{};
    const aiScene *scene = importer.ReadFile(path, IMPORT_PROPS);
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	throw std::runtime_error("failed to load model at \"" + path
				 + "\" assimp error: " + importer.GetErrorString());
    processNode(&model, scene->mRootNode, scene, aiMatrix4x4(), -1);

    for(auto &e: model.boneMap) {
	model.nodes[model.nodeMap[e.first]].boneID = e.second;
	model.nodes[model.nodeMap[e.first]].boneOffset = model.bones[e.second];
    }
    LOG("model bone count: " << model.bones.size());
    LOG("model animation count: " << scene->mNumAnimations);
    
    if(scene->HasAnimations()) {
	for(size_t i = 0; i < scene->mNumAnimations; i++) {
	    buildAnimations(&model, scene->mAnimations[i]);
	}
    }
    
    importer.FreeScene();
    return model;            
}

void AssimpLoader::processNode(ModelInfo::Model* model, aiNode* node,
			       const aiScene* scene, aiMatrix4x4 parentTransform, int parentNode) {
    aiMatrix4x4 transform = parentTransform * node->mTransformation;
    
    model->nodes.push_back(ModelInfo::Node{});
    model->nodes.back().parentNode = parentNode;
    model->nodes.back().transform = aiToGLM(node->mTransformation);

    int thisID = (int)model->nodes.size() - 1;
    model->nodeMap[node->mName.C_Str()] = thisID;
    if(parentNode >= 0) {
	model->nodes[parentNode].children.push_back(thisID);
    }
    
    for(unsigned int i = 0; i < node->mNumMeshes; i++) {
	aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
	model->meshes.push_back(ModelInfo::Mesh());
	processMesh(model, mesh, scene, transform);
    }
    
    for(unsigned int i = 0; i < node->mNumChildren; i++) {
	processNode(model, node->mChildren[i], scene, transform, thisID);
    }
}

std::vector<std::string> getTextures(aiMaterial* material, aiTextureType type);

glm::vec4 getColour(aiMaterial* material, const char* key, int type, int id);

void AssimpLoader::processMesh(ModelInfo::Model* model, aiMesh* aimesh,
			       const aiScene* scene, aiMatrix4x4 transform) {
    ModelInfo::Mesh* mesh = &model->meshes[model->meshes.size() - 1];
    mesh->bindTransform = aiToGLM(transform);

    auto material = scene->mMaterials[aimesh->mMaterialIndex];

    mesh->diffuseColour = getColour(material, AI_MATKEY_COLOR_DIFFUSE);
    mesh->diffuseTextures = getTextures(material, aiTextureType_DIFFUSE);
    
    //vertcies
    for(unsigned int i = 0; i < aimesh->mNumVertices;i++) {
	ModelInfo::Vertex vertex;
	vertex.Position.x = aimesh->mVertices[i].x;
	vertex.Position.y = aimesh->mVertices[i].y;
	vertex.Position.z = aimesh->mVertices[i].z;
	if(aimesh->HasNormals()) {
	    vertex.Normal.x = aimesh->mNormals[i].x;
	    vertex.Normal.y = aimesh->mNormals[i].y;
	    vertex.Normal.z = aimesh->mNormals[i].z;
	}
	else
	    vertex.Normal = glm::vec3(0);
	if(aimesh->mTextureCoords[0]) {
	    vertex.TexCoord.x = aimesh->mTextureCoords[0][i].x;
	    vertex.TexCoord.y = aimesh->mTextureCoords[0][i].y;
	}
	else
	    vertex.TexCoord = glm::vec2(0);

	mesh->verticies.push_back(vertex);
    }
    //bones - relies on verticies
    for(unsigned int i = 0; i < aimesh->mNumBones; i++) {
	auto aibone = aimesh->mBones[i];
	unsigned int boneID;
	std::string boneName = aibone->mName.C_Str();
	if(model->boneMap.find(boneName) == model->boneMap.end()) {
	    model->bones.push_back(aiToGLM(aibone->mOffsetMatrix) * mesh->bindTransform);
	    boneID = (unsigned int)(model->bones.size() - 1);
	    model->boneMap[boneName] = boneID;
	} else
	    boneID = model->boneMap[boneName];
	
	for(unsigned int weightI = 0; weightI < aibone->mNumWeights; weightI++) {
	    auto vertexWeight = aibone->mWeights[weightI];
	    mesh->verticies[vertexWeight.mVertexId].BoneIDs.push_back(boneID == -1 ? 0 : boneID);
	    mesh->verticies[vertexWeight.mVertexId].BoneWeights.push_back(vertexWeight.mWeight);
	}
    }
    //indicies
    for(unsigned int i = 0; i < aimesh->mNumFaces; i++) {
	aiFace face = aimesh->mFaces[i];
	for(unsigned int j = 0; j < face.mNumIndices; j++)
	    mesh->indices.push_back(face.mIndices[j]);
    }
}

void extractKeyframe(ModelInfo::AnimNodes *pNode, aiNodeAnim* pAssimpNode);

void AssimpLoader::buildAnimations(ModelInfo::Model* model, aiAnimation* aiAnim) {
    model->animations.push_back(ModelInfo::Animation());
    auto anim = &model->animations[model->animations.size() - 1];
    anim->name = aiAnim->mName.C_Str();
    
    //copy nodes from model
    anim->nodes.resize(model->nodes.size());
    for(int i = 0; i < model->nodes.size(); i++)
	anim->nodes[i].modelNode = model->nodes[i];

    anim->duration = aiAnim->mDuration;
    anim->ticks = aiAnim->mTicksPerSecond * 0.001; //for ms time
    if(anim->ticks == 0) {
	LOG_ERROR("WARNING: model animation does not specify ticks/ms, using 1/ms");
	anim->ticks = 1;
    }
    
    //set animation props for anim nodes
    for(unsigned int i = 0 ; i < aiAnim->mNumChannels; i++) {
	auto channel = aiAnim->mChannels[i];
	std::string nodeName = channel->mNodeName.C_Str();
	auto node = &anim->nodes[model->nodeMap[nodeName]];
	extractKeyframe(node, channel);
    }
}

/// --- Helpers ---

std::vector<std::string> getTextures(aiMaterial* material, aiTextureType type) {
    std::vector<std::string> textures;
    for(unsigned int i = 0; i < material->GetTextureCount(type); i++) {
	aiString texPath;
	material->GetTexture(type, i, &texPath);
	textures.push_back(texPath.C_Str());
	for(size_t i = 0; i < textures.back().size(); i++)
	    if(textures.back().at(i) == '\\')
		textures[textures.size() - 1][i] = '/';
    }
    return textures;
}

glm::vec4 getColour(aiMaterial* material, const char* key, int type, int id) {
    aiColor3D colour;
    if(material->Get(key, type, id, colour) != AI_SUCCESS) {
	LOG_ERROR("warning: failed to get colour type: " << type <<  " , setting to white\n");
	return glm::vec4(1.0f);
    }
    return glm::vec4(colour.r, colour.g, colour.b, 1);
}

void extractKeyframe(ModelInfo::AnimNodes *pNode, aiNodeAnim* pAssimpNode) {
    for(unsigned int posI = 0; posI < pAssimpNode->mNumPositionKeys; posI++) {
	auto posKey = &pAssimpNode->mPositionKeys[posI];
	ModelInfo::AnimationKey::Position pos;
	pos.Pos = glm::vec3(posKey->mValue.x, posKey->mValue.y, posKey->mValue.z);
	pos.time = posKey->mTime;
	pNode->positions.push_back(pos);
    }
    for(unsigned int rotI = 0; rotI < pAssimpNode->mNumRotationKeys; rotI++) {
	auto rotKey = &pAssimpNode->mRotationKeys[rotI];
	ModelInfo::AnimationKey::RotationQ rot;
	rot.Rot = glm::quat(rotKey->mValue.w, rotKey->mValue.x, rotKey->mValue.y, rotKey->mValue.z);
	rot.time = rotKey->mTime;
	pNode->rotationsQ.push_back(rot);
    }
    for(unsigned int scl = 0; scl < pAssimpNode->mNumScalingKeys; scl++) {
	auto scaleKey = &pAssimpNode->mScalingKeys[scl];
	ModelInfo::AnimationKey::Scaling scale;
	scale.scale = glm::vec3(scaleKey->mValue.x, scaleKey->mValue.y, scaleKey->mValue.z);
	scale.time = scaleKey->mTime;
	pNode->scalings.push_back(scale);
    }
}

inline glm::mat4 aiToGLM(aiMatrix4x4 mat) {
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

inline aiMatrix4x4 glmToAi(glm::mat4 mat) {
    aiMatrix4x4 aiMat = aiMatrix4x4(
	    mat[0][0], mat[0][1], mat[0][2], mat[0][3],
	    mat[1][0], mat[1][1], mat[1][2], mat[1][3],
	    mat[2][0], mat[2][1], mat[2][2], mat[2][3],
	    mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
    return aiMat;
}

#endif
