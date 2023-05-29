#include "model_render.h"

#include <stdint.h>
#include <array>
#include <stdexcept>
#include <cmath>
#include <cstring>
#include <iostream>

#include <glm/gtc/matrix_inverse.hpp>

#include "../vkhelper.h"
#include "../logger.h"
#include "../pipeline_data.h"

namespace Resource {
  struct MeshInfo {
      MeshInfo() { indexCount = 0; indexOffset = 0; vertexOffset = 0; }
      MeshInfo(uint32_t indexCount, uint32_t indexOffset, uint32_t vertexOffset,
	       Texture texture, glm::vec4 diffuseColour) {
	  this->indexCount = indexCount;
	  this->indexOffset = indexOffset;
	  this->vertexOffset = vertexOffset;
	  this->texture = texture;
	  this->diffuseColour = diffuseColour;
      }
      uint32_t indexCount;
      uint32_t indexOffset;
      uint32_t vertexOffset;
      Texture texture;
      glm::vec4 diffuseColour;
  };

  struct ModelInGPU {
      uint32_t vertexCount = 0;
      uint32_t indexCount  = 0;
      uint32_t vertexOffset = 0;
      uint32_t indexOffset = 0;
      std::vector<MeshInfo> meshes;

      std::vector<ModelAnimation> animations;
      std::map<std::string, int> animationMap;
      ModelType type;
  };
	
  ModelRender::ModelRender(DeviceState base, VkCommandPool pool) {
      this->base = base;
      this->pool = pool;
      loadQuad();
  }

  ModelRender::~ModelRender() {
      unloadAllModelData();
  }

  void ModelRender::unloadAllModelData() {
      if(models.size() <= 0)
	  return;
      models.clear();

      loaded2D.clearData();
      loaded3D.clearData();
      loadedAnim3D.clearData();
      
      alreadyLoaded.clear();
      vertexDataSize = 0;
      indexDataSize = 0;
      currentIndex = 0;
      vkDestroyBuffer(base.device, buffer, nullptr);
      vkFreeMemory(base.device, memory, nullptr);
  }

  void ModelRender::bindBuffers(VkCommandBuffer cmdBuff)
  {
      if(currentIndex == 0)
	  return;
      boundThisFrame = false;
      //bind index buffer - can only have one index buffer
      vkCmdBindIndexBuffer(cmdBuff, buffer, vertexDataSize, VK_INDEX_TYPE_UINT32);
  }

  void ModelRender::bindGroupVertexBuffer(VkCommandBuffer cmdBuff, ModelType type) {
      if(boundThisFrame && type == prevBoundType)
	  return;
      boundThisFrame = true;
      prevBoundType = type;
      size_t vOffset = 0;
      switch(type) {
      case ModelType::m2D:
	  vOffset = loaded2D.vertexDataOffset;
	  break;
      case ModelType::m3D:
	  vOffset = loaded3D.vertexDataOffset;
	  break;
      case ModelType::m3D_Anim:
	  vOffset = loadedAnim3D.vertexDataOffset;
	  break;
      }
      VkBuffer vertexBuffers[] = { buffer };
      VkDeviceSize offsets[] = { vOffset };
      vkCmdBindVertexBuffers(cmdBuff, 0, 1, vertexBuffers, offsets);
  }

  size_t ModelRender::getAnimationIndex(Model model, std::string animation) {
      if(models[model.ID].animationMap.find(animation) == models[model.ID].animationMap.end())
	  throw std::runtime_error("the animation " + animation + " could not be found on model");
      return models[model.ID].animationMap[animation];
  }

  ModelAnimation* ModelRender::getpAnimation(Model model, int animationIndex) {
      if(animationIndex >= models[model.ID].animations.size())
	  throw std::runtime_error("the animation index was out of range");
      return &models[model.ID].animations[animationIndex];
  }

  void ModelRender::drawModel(VkCommandBuffer cmdBuff, VkPipelineLayout layout, Model model,
			      uint32_t count, uint32_t instanceOffset) {
      if(model.ID >= models.size()) {
	  LOG("the model ID is out of range, ID: " << model.ID);
	  return;
      } 
      ModelInGPU *modelInfo = &models[model.ID];
      bindGroupVertexBuffer(cmdBuff, modelInfo->type);
      for(size_t i = 0; i < modelInfo->meshes.size(); i++) {
	  fragPushConstants fps {
	      modelInfo->meshes[i].diffuseColour,
	      glm::vec4(0, 0, 1, 1), //texOffset
	      (uint32_t)modelInfo->meshes[i].texture.ID
	  };
	  vkCmdPushConstants(cmdBuff, layout, VK_SHADER_STAGE_FRAGMENT_BIT,
			     0, sizeof(fragPushConstants), &fps);
	  drawMesh(cmdBuff, modelInfo, (uint32_t)i, count, instanceOffset);
      }
  }

  void ModelRender::drawQuad(VkCommandBuffer cmdBuff, VkPipelineLayout layout, unsigned int texID,
			     uint32_t count, uint32_t instanceOffset, glm::vec4 colour,
			     glm::vec4 texOffset) {
      bindGroupVertexBuffer(cmdBuff, ModelType::m2D);
      ModelInGPU *modelInfo = &models[static_cast<int>(quadID)];
      drawMesh(cmdBuff, modelInfo, 0, count, instanceOffset);
  }

  void ModelRender::drawMesh(VkCommandBuffer cmdBuff,
			     ModelInGPU *modelInfo,
			     uint32_t meshIndex, uint32_t instanceCount, uint32_t instanceOffset) {
    vkCmdDrawIndexed(
	      cmdBuff,
	      modelInfo->meshes[meshIndex].indexCount,
	      instanceCount,
	      modelInfo->meshes[meshIndex].indexOffset
	      + modelInfo->indexOffset,
	      modelInfo->meshes[meshIndex].vertexOffset
	      + modelInfo->vertexOffset,
	      instanceOffset);
  }

  template <class T_Vert>
  Model ModelRender::loadModelInfo(ModelInfo::Model& model, ModelGroup<T_Vert>& modelGroup, TextureLoader* texLoader) {
      Model userModel(currentIndex++);
      userModel.type = getModelType(T_Vert());
      modelGroup.loadModel(model, userModel.ID);
      loadModelTexture(modelGroup.getPreviousModel(), texLoader);
      return userModel;
  }

  Model ModelRender::loadModel(ModelType type, ModelInfo::Model& model, TextureLoader* texLoader, std::vector<Resource::ModelAnimation> *pGetAnimations) {
	switch(type) {
	case ModelType::m2D:
	    return loadModelInfo(model, loaded2D, texLoader);
	case ModelType::m3D:
	    return loadModelInfo(model, loaded3D, texLoader);
	case ModelType::m3D_Anim:
	    Model user_model = loadModelInfo(model, loadedAnim3D, texLoader);
	    for(ModelInfo::Animation anim : model.animations) {
		loadedAnim3D.getPreviousModel()->animations
		    .push_back(ModelAnimation(model.bones, anim));
		pGetAnimations->push_back(
			loadedAnim3D.getPreviousModel()->
			animations[loadedAnim3D.getPreviousModel()->animations.size() - 1]);
	    }
	    return user_model;
	}
	throw std::runtime_error("Model type not implemented when trying to load model!");
  }

  Model ModelRender::loadModel(ModelType type, std::string path, TextureLoader* texLoader, std::vector<Resource::ModelAnimation> *pGetAnimations) {
      ModelInfo::Model fileModel = loadModelFromFile(path);
      return loadModel(type, fileModel, texLoader, pGetAnimations);
  }

  ModelInfo::Model ModelRender::loadModelFromFile(std::string path) {
#ifndef NO_ASSIMP
      LOG("loading model: " << path);
      return modelLoader.LoadModel(path);
#else
      throw std::runtime_error("tried to load model from file, but NO_ASSIMP is defined"
"so that feature is disabled!");
#endif
  }

  void ModelRender::loadQuad() {
      ModelInfo::Mesh mesh;
      mesh.verticies.resize(4);
      mesh.verticies[0].Position = {0.0f, 0.0f, 0.0f};
      mesh.verticies[0].TexCoord = {0.0f, 0.0f};
      mesh.verticies[1].Position = {1.0f, 0.0f, 0.0f};
      mesh.verticies[1].TexCoord = {1.0f, 0.0f};
      mesh.verticies[2].Position = {1.0f, 1.0f, 0.0f};
      mesh.verticies[2].TexCoord = {1.0f, 1.0f};
      mesh.verticies[3].Position = {0.0f, 1.0f, 0.0f};
      mesh.verticies[3].TexCoord = {0.0f, 1.0f};
      mesh.indicies = { 0, 3, 2, 2, 1, 0};
      ModelInfo::Model quad;
      quad.meshes.push_back(mesh);
      quadID = loadModel(ModelType::m2D, quad, nullptr, nullptr).ID;
  }
  
  template <class T_Vert>
  void ModelRender::loadModelTexture(LoadedModel<T_Vert> *model, TextureLoader* texLoader) {
      for(auto& mesh: model->meshes) {
	  if(mesh->texToLoad != "") {
	      std::string loadStr = checkTextureLoaded(mesh->texToLoad, alreadyLoaded,
						     &mesh->texture);
	      if(loadStr != "")
		  mesh->texture = texLoader->LoadTexture(loadStr);
	  }
      }
  }

  void ModelRender::endLoading(VkCommandBuffer transferBuff) {
      if(loaded2D.models.size() == 0  &&
	 loaded3D.models.size() == 0 &&
	 loadedAnim3D.models.size() == 0) {
	  LOG("no model data to load to gpu");
	  return;
      }

      //get size of vertex data + offsets
      processLoadGroup(&loaded2D);
      processLoadGroup(&loaded3D);
      processLoadGroup(&loadedAnim3D);

      LOG("finished processing model groups");

      //load to staging buffer
      VkBuffer stagingBuffer;
      VkDeviceMemory stagingMemory;

      if(vkhelper::createBufferAndMemory(
		 base, vertexDataSize + indexDataSize, &stagingBuffer, &stagingMemory,
		 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
	 != VK_SUCCESS) {
	  throw std::runtime_error("Failed to create staging buffer for model data");
      }

      vkBindBufferMemory(base.device, stagingBuffer, stagingMemory, 0);
      void* pMem;
      vkMapMemory(base.device, stagingMemory, 0, vertexDataSize + indexDataSize, 0, &pMem);

      //copy each model's data to staging memory
      size_t currentVertexOffset = 0;
      size_t currentIndexOffset = vertexDataSize;

      stageLoadGroup(pMem, &loaded2D, currentVertexOffset, currentIndexOffset);
      stageLoadGroup(pMem, &loaded3D, currentVertexOffset, currentIndexOffset);
      stageLoadGroup(pMem, &loadedAnim3D, currentVertexOffset, currentIndexOffset);

      LOG("finished staging model groups");

      //create final dest memory
      vkhelper::createBufferAndMemory(base, vertexDataSize + indexDataSize, &buffer, &memory,
				      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
				      VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
				      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

      vkBindBufferMemory(base.device, buffer, memory, 0);

      //copy from staging buffer to final memory location

      VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
      beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      vkBeginCommandBuffer(transferBuff, &beginInfo);

      VkBufferCopy copyRegion{};
      copyRegion.srcOffset = 0;
      copyRegion.dstOffset = 0;
      copyRegion.size = vertexDataSize + indexDataSize;
      vkCmdCopyBuffer(transferBuff, stagingBuffer, buffer, 1, &copyRegion);
      vkEndCommandBuffer(transferBuff);

      VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &transferBuff;
      vkQueueSubmit(base.queue.graphicsPresentQueue, 1, &submitInfo, VK_NULL_HANDLE);
      vkQueueWaitIdle(base.queue.graphicsPresentQueue);

      //free staging buffer
      vkDestroyBuffer(base.device, stagingBuffer, nullptr);
      vkFreeMemory(base.device, stagingMemory, nullptr);

      LOG("finished loading model data to gpu");
  }

  template <class T_Vert >
  void ModelRender::processLoadGroup(ModelGroup<T_Vert>* pGroup) {
    pGroup->vertexDataOffset = vertexDataSize;
    uint32_t modelVertexOffset = 0;
    for(size_t i = 0; i < pGroup->models.size(); i++) {
      models[pGroup->models[i].ID] = ModelInGPU();
      ModelInGPU* model = &models[pGroup->models[i].ID];

      model->type = getModelType(pGroup->models[i].meshes[0]->verticies[0]);
      model->vertexOffset = modelVertexOffset;
      model->indexOffset = indexDataSize / sizeof(pGroup->models[i].meshes[0]->indicies[0]);
      model->meshes.resize(pGroup->models[i].meshes.size());
      for(size_t j = 0 ; j <  pGroup->models[i].meshes.size(); j++) {
	model->meshes[j] = MeshInfo(
				    (uint32_t)pGroup->models[i].meshes[j]->indicies.size(),
				    model->indexCount,  //as offset
				    model->vertexCount, //as offset
				    pGroup->models[i].meshes[j]->texture,
				    pGroup->models[i].meshes[j]->diffuseColour);
	model->vertexCount += (uint32_t)pGroup->models[i].meshes[j]->verticies.size();
	model->indexCount  += (uint32_t)pGroup->models[i].meshes[j]->indicies.size();
	vertexDataSize += sizeof(T_Vert) * (uint32_t)pGroup->models[i].meshes[j]->verticies.size();
	indexDataSize +=  sizeof(pGroup->models[i].meshes[j]->indicies[0])
	  * (uint32_t)pGroup->models[i].meshes[j]->indicies.size();
      }
      modelVertexOffset += model->vertexCount;

      for(size_t anim = 0; anim < pGroup->models[i].animations.size(); anim++) {
	model->animations.push_back(pGroup->models[i].animations[anim]);
	model->animationMap[pGroup->models[i].animations[anim].getName()] = static_cast<int>(
											     pGroup->models[i].animations.size() - 1);
      }

    }
    pGroup->vertexDataSize = vertexDataSize - pGroup->vertexDataOffset;
  }

  template <class T_Vert >
  void ModelRender::stageLoadGroup(void* pMem, ModelGroup<T_Vert >* pGroup,
				   size_t &pVertexDataOffset, size_t &pIndexDataOffset) {
    for(auto& model: pGroup->models) {
      for(size_t i = 0; i < model.meshes.size(); i++) {
		    
	std::memcpy(static_cast<char*>(pMem) + pVertexDataOffset,
		    model.meshes[i]->verticies.data(),
		    sizeof(T_Vert ) * model.meshes[i]->verticies.size());
			
	pVertexDataOffset += sizeof(T_Vert ) * model.meshes[i]->verticies.size();
			
	std::memcpy(static_cast<char*>(pMem) + pIndexDataOffset,
		    model.meshes[i]->indicies.data(),
		    sizeof(model.meshes[i]->indicies[0])
		    * model.meshes[i]->indicies.size());
			
	pIndexDataOffset += sizeof(model.meshes[i]->indicies[0])
	  * model.meshes[i]->indicies.size();
			
	delete model.meshes[i];
      }
    }
    pGroup->models.clear();
  }
  } //end namespace
