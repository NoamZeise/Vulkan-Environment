#include <resource_loader/model_loader.h>
#include <graphics/logger.h>

#include <stdexcept>

InternalModelLoader::InternalModelLoader(Resource::Pool pool, InternalTexLoader* texLoader) {
    this->texLoader = texLoader;
    this->pool = pool;
}

InternalModelLoader::~InternalModelLoader() {
    clearStaged();
    clearGPU();
}

void InternalModelLoader::clearStaged() {
    stage2D.clearData();
    stage3D.clearData();
    stageAnim3D.clearData();
    currentIndex = 0;
}

Resource::Model InternalModelLoader::LoadModel(
	Resource::ModelType type, std::string path,
	std::vector<Resource::ModelAnimation> *pAnimations) {
    ModelInfo::Model model = loader.LoadModel(path);
    return LoadModel(type, model, pAnimations);
}

Resource::Model InternalModelLoader::LoadModel(
	Resource::ModelType type, ModelInfo::Model &modelData,
	std::vector<Resource::ModelAnimation>* pAnimations) {
    switch(type) {
    case Resource::ModelType::m2D:
	return load(modelData, stage2D, pAnimations);
    case Resource::ModelType::m3D:
	return load(modelData, stage3D, pAnimations);
    case Resource::ModelType::m3D_Anim:
	return load(modelData, stageAnim3D, pAnimations);
    default:
	throw std::runtime_error("Model Type Not implemented in "
				 "InternalModelLoader");
    }
}

template <class T_Vert>
Resource::Model InternalModelLoader::load(ModelInfo::Model& model,
					  ModelGroup<T_Vert>& modelGroup,
					  std::vector<Resource::ModelAnimation> *pAnimations) {
    Resource::Model usermodel(currentIndex++, getModelType(T_Vert()), pool);
    modelGroup.loadModel(model, usermodel.ID);
    LoadedModel<T_Vert>* loaded = modelGroup.getPreviousModel();

    for(Mesh<T_Vert> *mesh: loaded->meshes) {
	if(mesh->texToLoad != "")
	    mesh->texture = texLoader->LoadTexture(MODEL_TEXTURE_LOCATION
						   + mesh->texToLoad);
	else
	    mesh->texture.ID = UINT32_MAX;
    }
    
    for(ModelInfo::Animation &anim: model.animations) {
	loaded->animations.push_back(Resource::ModelAnimation(model.bones, anim));
	if(pAnimations != nullptr)
	    pAnimations->push_back(loaded->animations[loaded->animations.size() - 1]);
    }
    
    LOG("Model Loaded " <<
	" - pool: " << pool.ID <<
	" - id: " << usermodel.ID);
    return usermodel;
}

void InternalModelLoader::loadQuad() {
    ModelInfo::Model q = makeQuadModel();
    quad = LoadModel(Resource::ModelType::m2D, q, nullptr);
}

#ifndef NO_ASSIMP
#include <assimp/anim.h>
#include <assimp/material.h>
#include <assimp/matrix4x4.h>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>

namespace Resource
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

AssimpLoader::AssimpLoader() {}

ModelInfo::Model AssimpLoader::LoadModel(std::string path) {
    auto model = ModelInfo::Model{};

    const aiScene *scene = importer.ReadFile(path, IMPORT_PROPS);

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	throw std::runtime_error("failed to load model at \"" + path
				 + "\" assimp error: " + importer.GetErrorString());
    
    processNode(&model, scene->mRootNode, scene, aiMatrix4x4(), -1);
    LOG("model bone count: " << model.bones.size());
    LOG("model animation count: " << scene->mNumAnimations);

    if(scene->HasAnimations()) {
        model.animatedModel = true;
        for(size_t i = 0; i < scene->mNumAnimations; i++) {
            LOG("loading animation: " << scene->mAnimations[i]->mName.C_Str());
            buildAnimation(&model, scene->mAnimations[i]);
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
      int thisID = static_cast<int>(model->nodes.size() - 1);
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
  
  void AssimpLoader::processMesh(ModelInfo::Model* model, aiMesh* aimesh,
				 const aiScene* scene, aiMatrix4x4 transform) {
      ModelInfo::Mesh* mesh = &model->meshes[model->meshes.size() - 1];
      mesh->bindTransform = aiToGLM(transform);

      auto material = scene->mMaterials[aimesh->mMaterialIndex];

      aiColor3D diffuseColour;
      if(material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColour) != AI_SUCCESS) {
	  LOG_ERROR("warning: failed to get diffuse colour\n");
	  mesh->diffuseColour = glm::vec4(1);
      }
      else
	  mesh->diffuseColour = glm::vec4(diffuseColour.r, diffuseColour.g, diffuseColour.b, 1);

      for(unsigned int i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); i++) {
	  aiString texPath;
	  material->GetTexture(aiTextureType_DIFFUSE, i, &texPath);
	  mesh->diffuseTextures.push_back(texPath.C_Str());
	  for(size_t i = 0; i < mesh->diffuseTextures.back().size(); i++)
	      if(mesh->diffuseTextures.back().at(i) == '\\')
		  mesh->diffuseTextures[mesh->diffuseTextures.size() - 1][i] = '/';
      }

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
	  if(model->boneMap.find(boneName) == model->boneMap.end()){
	      model->bones.push_back(aiToGLM(aibone->mOffsetMatrix));
	      boneID = static_cast<unsigned int>(model->bones.size() - 1);
	      model->boneMap[boneName] = boneID;
	  } else
	      boneID = model->boneMap[boneName];

	  for(unsigned int bone = 0; bone < aibone->mNumWeights; bone++) {
	      auto vertexWeight = aibone->mWeights[bone];
	      mesh->verticies[vertexWeight.mVertexId].BoneIDs.push_back(boneID);
	      mesh->verticies[vertexWeight.mVertexId].BoneWeights.push_back(vertexWeight.mWeight);
	  }
      }

      //indicies
      for(unsigned int i = 0; i < aimesh->mNumFaces; i++) {
	  aiFace face = aimesh->mFaces[i];
	  for(unsigned int j = 0; j < face.mNumIndices; j++)
	      mesh->indicies.push_back(face.mIndices[j]);
      }
  }

  void AssimpLoader::buildAnimation(ModelInfo::Model* model, aiAnimation* aiAnim) {
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
      if(anim->ticks == 0) {
	  LOG_ERROR("WARNING: format does not specify ticks, using 100/s\n");
	  anim->ticks = 100;
      }

      //set animation props for anim nodes
      for(unsigned int i = 0 ; i < aiAnim->mNumChannels; i++) {
	  auto channel = aiAnim->mChannels[i];
	  std::string nodeName = channel->mNodeName.C_Str();
	  auto node = &anim->nodes[model->nodeMap[nodeName]];
	  if(model->boneMap.find(nodeName) != model->boneMap.end()) {
	      node->boneID = model->boneMap[nodeName];
	      node->boneOffset = model->bones[node->boneID];
	  }
	  //else bone directly affects no nodes;

	  for(unsigned int pos = 0; pos < channel->mNumPositionKeys; pos++) {
	      auto posKey = channel->mPositionKeys[pos];
	      node->positions.push_back(ModelInfo::AnimationKey::Position{});
	      node->positions.back().Pos = glm::vec3(posKey.mValue.x, posKey.mValue.y, posKey.mValue.z);
	      node->positions.back().time = posKey.mTime;
	  }

	  for(unsigned int rot = 0; rot < channel->mNumRotationKeys; rot++) {
	      auto rotKey = channel->mRotationKeys[rot];
	      node->rotationsQ.push_back(ModelInfo::AnimationKey::RotationQ{});
	      node->rotationsQ.back().Rot = glm::quat(rotKey.mValue.w, rotKey.mValue.x, rotKey.mValue.y, rotKey.mValue.z);
	      node->rotationsQ.back().time = rotKey.mTime;
	  }

	  for(unsigned int scl = 0; scl < channel->mNumScalingKeys; scl++) {
	      auto scaleKey = channel->mScalingKeys[scl];
	      node->scalings.push_back(ModelInfo::AnimationKey::Scaling{});
	      node->scalings.back().scale = glm::vec3(scaleKey.mValue.x, scaleKey.mValue.y, scaleKey.mValue.z);
	      node->scalings.back().time = scaleKey.mTime;
	  }
      }
  }
} // namespace Resource

#endif
