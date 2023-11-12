#include "model_render.h"

#include <stdint.h>
#include <array>
#include <stdexcept>
#include <cmath>
#include <cstring>

#include <glm/gtc/matrix_inverse.hpp>

#include "../vkhelper.h"
#include "../logger.h"
#include "../pipeline_data.h"

  struct MeshInfo {
      MeshInfo() { indexCount = 0; indexOffset = 0; vertexOffset = 0; }
      MeshInfo(uint32_t indexCount, uint32_t indexOffset, uint32_t vertexOffset,
	       Resource::Texture texture, glm::vec4 diffuseColour) {
	  this->indexCount = indexCount;
	  this->indexOffset = indexOffset;
	  this->vertexOffset = vertexOffset;
	  this->texture = texture;
	  this->diffuseColour = diffuseColour;
      }
      uint32_t indexCount;
      uint32_t indexOffset;
      uint32_t vertexOffset;
      Resource::Texture texture;
      glm::vec4 diffuseColour;
  };

  struct ModelInGPU {
      uint32_t vertexCount = 0;
      uint32_t indexCount  = 0;
      uint32_t vertexOffset = 0;
      uint32_t indexOffset = 0;
      std::vector<MeshInfo> meshes;

      std::vector<Resource::ModelAnimation> animations;
      std::map<std::string, int> animationMap;
      Resource::ModelType type;
  };
	
ModelLoaderVk::ModelLoaderVk(DeviceState base, VkCommandPool cmdpool,
			     Resource::Pool pool, InternalTexLoader* texLoader)
    : InternalModelLoader(pool, texLoader){
      this->base = base;
      this->cmdpool = cmdpool;
  }

  void ModelLoaderVk::clearGPU() {
      if(models.empty())
	  return;
      for(ModelInGPU* model: models)
	  delete model;
      models.clear();
      
      vertexDataSize = 0;
      indexDataSize = 0;
      
      vkDestroyBuffer(base.device, buffer, nullptr);
      vkFreeMemory(base.device, memory, nullptr);
  }

  void ModelLoaderVk::bindBuffers(VkCommandBuffer cmdBuff) {
      boundThisFrame = false;
      //bind index buffer - can only have one index buffer
      vkCmdBindIndexBuffer(cmdBuff, buffer, vertexDataSize, VK_INDEX_TYPE_UINT32);
  }

void ModelLoaderVk::bindGroupVertexBuffer(VkCommandBuffer cmdBuff, Resource::ModelType type) {
    if(boundThisFrame && type == prevBoundType)
	return;
    boundThisFrame = true;
    prevBoundType = type;
    size_t vOffset = modelTypeOffset[(size_t)type];
    VkBuffer vertexBuffers[] = { buffer };
    VkDeviceSize offsets[] = { vOffset };
    vkCmdBindVertexBuffers(cmdBuff, 0, 1, vertexBuffers, offsets);
}

Resource::ModelAnimation ModelLoaderVk::getAnimation(Resource::Model model, std::string animation) {
    if(models[model.ID]->animationMap.find(animation) == models[model.ID]->animationMap.end())
	throw std::runtime_error("the animation " + animation + " could not be found on model");
    return getAnimation(model, models[model.ID]->animationMap[animation]);
}

Resource::ModelAnimation ModelLoaderVk::getAnimation(Resource::Model model, int animationIndex) {
    if(animationIndex >= models[model.ID]->animations.size())
	throw std::runtime_error("the animation index was out of range");
    return models[model.ID]->animations[animationIndex];
}

void ModelLoaderVk::drawModel(VkCommandBuffer cmdBuff, VkPipelineLayout layout,
			      Resource::Model model,
			      uint32_t count, uint32_t instanceOffset, glm::vec4 colour) {
    if(model.ID >= models.size()) {
	LOG("the model ID is out of range, ID: " << model.ID);
	return;
    } 
    ModelInGPU *modelInfo = models[model.ID];
    bindGroupVertexBuffer(cmdBuff, modelInfo->type);
    for(size_t i = 0; i < modelInfo->meshes.size(); i++) {
	size_t texID = modelInfo->meshes[i].texture.ID; 
	if(texID != UINT32_MAX) 
	    texID = texLoader->getViewIndex(texID);
	else
	    texID = 0;
	fragPushConstants fps {
	    colour.a == 0.0f ? modelInfo->meshes[i].diffuseColour : colour,
	    glm::vec4(0, 0, 1, 1), //texOffset
	    (uint32_t)texID,
	};
	vkCmdPushConstants(cmdBuff, layout, VK_SHADER_STAGE_FRAGMENT_BIT,
			   0, sizeof(fragPushConstants), &fps);
	drawMesh(cmdBuff, modelInfo, (uint32_t)i, count, instanceOffset);
    }
}

  void ModelLoaderVk::drawQuad(VkCommandBuffer cmdBuff, VkPipelineLayout layout, unsigned int texID,
			     uint32_t count, uint32_t instanceOffset, glm::vec4 colour,
			     glm::vec4 texOffset) {
      bindGroupVertexBuffer(cmdBuff, Resource::ModelType::m2D);
      ModelInGPU *modelInfo = models[static_cast<int>(quadID)];
      drawMesh(cmdBuff, modelInfo, 0, count, instanceOffset);
  }

  void ModelLoaderVk::drawMesh(VkCommandBuffer cmdBuff,
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

  void ModelLoaderVk::loadQuad() {
      ModelInfo::Model quad = makeQuadModel();
      quadID = LoadModel(Resource::ModelType::m2D, quad, nullptr).ID;
  }

  void ModelLoaderVk::loadGPU(VkCommandBuffer transferBuff) {
      loadQuad();
      int modelCount = currentIndex;
      clearGPU();
      LOG("Model count: " << modelCount);
      models.resize(modelCount);
      //get size of vertex data + offsets
      processLoadGroup(&stage2D);
      processLoadGroup(&stage3D);
      processLoadGroup(&stageAnim3D);

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

      stageLoadGroup(pMem, &stage2D, currentVertexOffset, currentIndexOffset);
      stageLoadGroup(pMem, &stage3D, currentVertexOffset, currentIndexOffset);
      stageLoadGroup(pMem, &stageAnim3D, currentVertexOffset, currentIndexOffset);
      clearStaged();

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
  void ModelLoaderVk::processLoadGroup(ModelGroup<T_Vert>* pGroup) {
      T_Vert vert = T_Vert();
      Resource::ModelType type = getModelType(vert);
      modelTypeOffset[(size_t)type] = vertexDataSize;
      pGroup->vertexDataOffset = vertexDataSize;
      uint32_t modelVertexOffset = 0;
      for(size_t i = 0; i < pGroup->models.size(); i++) {
	  models[pGroup->models[i].ID] = new ModelInGPU();

	  ModelInGPU* model = models[pGroup->models[i].ID];

	  model->type = type;
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
	      vertexDataSize += sizeof(T_Vert)
		  * (uint32_t)pGroup->models[i].meshes[j]->verticies.size();
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
  void ModelLoaderVk::stageLoadGroup(void* pMem, ModelGroup<T_Vert >* pGroup,
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
