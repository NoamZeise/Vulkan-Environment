#include "model_render.h"
#include "assimp/mesh.h"
#include <stdexcept>

namespace Resource
{

ModelRender::ModelRender(Base base, VkCommandPool pool)
{
	this->base = base;
	this->pool = pool;

	loadQuad();
}

ModelRender::~ModelRender()
{
	if(models.size() <= 0)
		return;
	for (auto& model : loaded2D.models)
			for (size_t i = 0; i < model.meshes.size(); i++)
				delete model.meshes[i];
	for (auto& model : loaded3D.models)
			for (size_t i = 0; i < model.meshes.size(); i++)
				delete model.meshes[i];
	for (auto& model : loadedAnim3D.models)
			for (size_t i = 0; i < model.meshes.size(); i++)
				delete model.meshes[i];

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

void ModelRender::bindGroupVertexBuffer(VkCommandBuffer cmdBuff, ModelType type)
{
	if(boundThisFrame && type == prevBoundType)
		return;
	boundThisFrame = true;
	prevBoundType = type;
	size_t vOffset = 0;
	switch(type)
	{
		case ModelType::model2D:
			vOffset = loaded2D.vertexDataOffset;
			break;
		case ModelType::model3D:
			vOffset = loaded3D.vertexDataOffset;
			break;
		case ModelType::modelAnim3D:
			vOffset = loadedAnim3D.vertexDataOffset;
			break;
	}
	VkBuffer vertexBuffers[] = { buffer };
	VkDeviceSize offsets[] = { vOffset };
	vkCmdBindVertexBuffers(cmdBuff, 0, 1, vertexBuffers, offsets);
}

int ModelRender::getAnimationIndex(Model model, std::string animation)
{
	if(models[model.ID].animationMap.find(animation) == models[model.ID].animationMap.end())
		throw std::runtime_error("the animation " + animation + " could not be found on model");
	return models[model.ID].animationMap[animation];
}

ModelAnimation* ModelRender::getpAnimation(Model model, int animationIndex)
{
	if(animationIndex >= models[model.ID].animations.size())
		throw std::runtime_error("the animation index was out of range");
	return &models[model.ID].animations[animationIndex];
}

void ModelRender::drawModel(VkCommandBuffer cmdBuff, VkPipelineLayout layout, Model model, size_t count, size_t instanceOffset)
{
	if(model.ID >= models.size())
	{
		std::cout << "the model ID is out of range, ID: " << model.ID << std::endl;
		return;
	}

	ModelInGPU *modelInfo = &models[model.ID];
	bindGroupVertexBuffer(cmdBuff, modelInfo->type);
	for(size_t i = 0; i < modelInfo->meshes.size(); i++)
	{
		fragPushConstants fps{
			modelInfo->meshes[i].diffuseColour,
			glm::vec4(0, 0, 1, 1), //texOffset
			modelInfo->meshes[i].texture.ID
		};

		vkCmdPushConstants(cmdBuff, layout, VK_SHADER_STAGE_FRAGMENT_BIT,
			0, sizeof(fragPushConstants), &fps);

		vkCmdDrawIndexed(
			cmdBuff,
			static_cast<uint32_t>(modelInfo->meshes[i].indexCount),
			static_cast<uint32_t>(count),
			static_cast<uint32_t>(modelInfo->meshes[i].indexOffset + modelInfo->indexOffset),
			static_cast<uint32_t>(modelInfo->meshes[i].vertexOffset + modelInfo->vertexOffset),
		    static_cast<uint32_t>(instanceOffset)
            );
	}
}

void ModelRender::drawQuad(VkCommandBuffer cmdBuff, VkPipelineLayout layout, unsigned int texID, size_t count, size_t instanceOffset, glm::vec4 colour, glm::vec4 texOffset)
{
		bindGroupVertexBuffer(cmdBuff, ModelType::model2D);

		ModelInGPU *modelInfo = &models[static_cast<int>(quadID)];
		vkCmdDrawIndexed(cmdBuff,
			static_cast<uint32_t>(modelInfo->meshes[0].indexCount),
			static_cast<uint32_t>(count),
			static_cast<uint32_t>(modelInfo->meshes[0].indexOffset + modelInfo->indexOffset),
			static_cast<uint32_t>(modelInfo->meshes[0].vertexOffset + modelInfo->vertexOffset),
			static_cast<uint32_t>(instanceOffset)
        );
}

void ModelRender::loadQuad()
{
	loaded2D.models.push_back(LoadedModel<Vertex2D>());
	auto ldModel = &loaded2D.models[loaded2D.models.size() - 1];
	ldModel->directory = "quad";
	quadID = currentIndex;
	ldModel->ID = static_cast<uint32_t>(currentIndex);
	ldModel->meshes.push_back(new Mesh<Vertex2D>());
	auto mesh = ldModel->meshes[ldModel->meshes.size() - 1];
	mesh->texture =  Texture(0, glm::vec2(0,0), "quad");
	mesh->verticies =
		{
			//pos   	  			texcoord
			{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
			{{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
			{{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
			{{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
		};
	mesh->indicies = { 0, 3, 2, 2, 1, 0};
    currentIndex++;
}

Model ModelRender::loadModel(std::string path, TextureLoader* texLoader, std::vector<Resource::ModelAnimation> *pGetAnimations)
{
#ifndef NO_ASSIMP

#ifndef NDEBUG
    std::cout << "\nloading model: " << path << std::endl;
#endif

	ModelInfo::Model loadM = modelLoader.LoadModel(path);

	Model model(static_cast<uint32_t>(currentIndex));

	//TODO reuse more for animated vs static
	if(loadM.animations.size() == 0 || pGetAnimations == nullptr)
	{
		loaded3D.models.push_back(LoadedModel<Vertex3D>());
		auto ldModel = &loaded3D.models[loaded3D.models.size() - 1];
		ldModel->directory = path.substr(0, path.find_last_of('/'));
		ldModel->ID = static_cast<uint32_t>(currentIndex);
		for(int mesh = 0; mesh < loadM.meshes.size(); mesh++)
		{
			ldModel->meshes.push_back(new Mesh<Vertex3D>{});
			auto ldMesh = ldModel->meshes.back();

		// just take one diffuse texture for now
		// TODO support multiple textures
			if(loadM.meshes[mesh].diffuseTextures.size() > 0)
				ldMesh->texture = loadTexture(loadM.meshes[mesh].diffuseTextures[0], texLoader);

			ldMesh->diffuseColour = loadM.meshes[mesh].diffuseColour;

			glm::mat4 meshTransform = loadM.meshes[mesh].bindTransform;
			for(int vert = 0; vert < loadM.meshes[mesh].verticies.size(); vert++)
			{
				Vertex3D vertex;
				vertex.Position = meshTransform * glm::vec4(loadM.meshes[mesh].verticies[vert].Position, 1.0f);
				vertex.Normal = glm::mat3(glm::inverseTranspose(meshTransform)) * loadM.meshes[mesh].verticies[vert].Normal;
				vertex.TexCoord = loadM.meshes[mesh].verticies[vert].TexCoord;
				ldMesh->verticies.push_back(vertex);

			}

			ldMesh->indicies = loadM.meshes[mesh].indicies;
		}
	}
	else
	{
		loadedAnim3D.models.push_back(LoadedModel<VertexAnim3D>());
		auto ldModel = &loadedAnim3D.models[loadedAnim3D.models.size() - 1];
		ldModel->directory = path.substr(0, path.find_last_of('/'));
		ldModel->ID = static_cast<uint32_t>(currentIndex);
		for(auto& mesh : loadM.meshes)
		{
			ldModel->meshes.push_back(new Mesh<VertexAnim3D>{});
			auto ldMesh = ldModel->meshes.back();

		// just take one diffuse texture for now
		// TODO support multiple textures
			if(mesh.diffuseTextures.size() > 0)
				ldMesh->texture = loadTexture(mesh.diffuseTextures[0], texLoader);

			ldMesh->diffuseColour = mesh.diffuseColour;


			for(int vert = 0; vert < mesh.verticies.size(); vert++)
			{
				VertexAnim3D vertex;
				vertex.Position = glm::vec4(mesh.verticies[vert].Position, 1.0f);
				vertex.Normal = mesh.verticies[vert].Normal;
				vertex.TexCoord = mesh.verticies[vert].TexCoord;
				for(int vecElem = 0; vecElem < 4; vecElem++)
				{
					if(mesh.verticies[vert].BoneIDs.size() <= vecElem)
					{
						vertex.BoneIDs[vecElem] = -1;
						vertex.Weights[vecElem] = 0;
					}
					else
					{
						vertex.BoneIDs[vecElem] = mesh.verticies[vert].BoneIDs[vecElem];
						vertex.Weights[vecElem] = mesh.verticies[vert].BoneWeights[vecElem];
					}
				}
				if(mesh.verticies[vert].BoneIDs.size() > 4)
					std::cout << "vertex influenced by more than 4 bones, but only 4 bones will be used!\n";
				ldMesh->verticies.push_back(vertex);
			}

			ldMesh->indicies = mesh.indicies;
		}
		for(const auto &anim : loadM.animations)
		{
			ldModel->animations.push_back(ModelAnimation(loadM.bones, anim));
			pGetAnimations->push_back(ldModel->animations[ldModel->animations.size() - 1]);
		}
	}
	currentIndex++;
#ifndef NDEBUG
	std::cout << "finished loading model\n\n";
#endif

	return model;
#else
	throw std::runtime_error("tried to load model but NO_ASSIMP is defined!");
#endif
}

Model ModelRender::loadModel(std::string path, TextureLoader* texLoader)
{
	return loadModel(path, texLoader, nullptr);
}


#ifndef NO_ASSIMP

Resource::Texture ModelRender::loadTexture(std::string path, TextureLoader* texLoader)
{
	std::string texLocation = MODEL_TEXTURE_LOCATION + path;

	for(unsigned int i = 0; i < alreadyLoaded.size(); i++)
		if(std::strcmp(alreadyLoaded[i].path.data(), texLocation.c_str()) == 0)
			return alreadyLoaded[i];

	alreadyLoaded.push_back(texLoader->LoadTexture(texLocation));
	alreadyLoaded[alreadyLoaded.size() - 1].type = TextureType::Diffuse; //attention
	return alreadyLoaded[alreadyLoaded.size() - 1];
}
#endif

template <class T_Vert >
void ModelRender::processLoadGroup(ModelGroup<T_Vert>* pGroup)
{
	pGroup->vertexDataOffset = vertexDataSize;
	size_t modelVertexOffset = 0;
	for(size_t i = 0; i < pGroup->models.size(); i++)
	{
		models[pGroup->models[i].ID] = ModelInGPU();
		ModelInGPU* model = &models[pGroup->models[i].ID];

		model->type = getModelType(pGroup->models[i].meshes[0]->verticies[0]);
		model->vertexOffset = modelVertexOffset;
		model->indexOffset = indexDataSize / sizeof(pGroup->models[i].meshes[0]->indicies[0]);
		model->meshes.resize(pGroup->models[i].meshes.size());
		for(size_t j = 0 ; j <  pGroup->models[i].meshes.size(); j++)
		{
			model->meshes[j] = MeshInfo(
					pGroup->models[i].meshes[j]->indicies.size(),
					model->indexCount,  //as offset
					model->vertexCount, //as offset
					pGroup->models[i].meshes[j]->texture,
					pGroup->models[i].meshes[j]->diffuseColour);
			model->vertexCount += pGroup->models[i].meshes[j]->verticies.size();
			model->indexCount  += pGroup->models[i].meshes[j]->indicies.size();
			vertexDataSize += sizeof(T_Vert) * pGroup->models[i].meshes[j]->verticies.size();
			indexDataSize +=  sizeof(pGroup->models[i].meshes[j]->indicies[0]) * pGroup->models[i].meshes[j]->indicies.size();
		}
		modelVertexOffset += model->vertexCount;

		for(size_t anim = 0; anim < pGroup->models[i].animations.size(); anim++)
		{
			model->animations.push_back(pGroup->models[i].animations[anim]);
			model->animationMap[pGroup->models[i].animations[anim].getName()] = static_cast<int>(pGroup->models[i].animations.size() - 1);
		}

	}
	pGroup->vertexDataSize = vertexDataSize - pGroup->vertexDataOffset;
}

template <class T_Vert >
void ModelRender::stageLoadGroup(void* pMem, ModelGroup<T_Vert >* pGroup, size_t &pVertexDataOffset, size_t &pIndexDataOffset)
{
	for(auto& model: pGroup->models)
	{
		for(size_t i = 0; i < model.meshes.size(); i++)
		{
			std::memcpy(static_cast<char*>(pMem) + pVertexDataOffset, model.meshes[i]->verticies.data(), sizeof(T_Vert ) * model.meshes[i]->verticies.size());
			pVertexDataOffset += sizeof(T_Vert ) * model.meshes[i]->verticies.size();
			std::memcpy(static_cast<char*>(pMem) + pIndexDataOffset, model.meshes[i]->indicies.data(), sizeof(model.meshes[i]->indicies[0]) * model.meshes[i]->indicies.size());
			pIndexDataOffset += sizeof(model.meshes[i]->indicies[0]) * model.meshes[i]->indicies.size();
			delete model.meshes[i];
		}
	}
	pGroup->models.clear();
}

void ModelRender::endLoading(VkCommandBuffer transferBuff)
{
	if(loaded2D.models.size() == 0  && loaded3D.models.size() == 0 && loadedAnim3D.models.size() == 0)
	{
		std::cout << "no model data to load to gpu" << std::endl;
		return;
	}

	//get size of vertex data + offsets
	processLoadGroup(&loaded2D);
	processLoadGroup(&loaded3D);
	processLoadGroup(&loadedAnim3D);

#ifndef NDEBUG
	std::cout << "finished processing model groups" << std::endl;
#endif


	//load to staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;

	vkhelper::createBufferAndMemory(base, vertexDataSize + indexDataSize, &stagingBuffer, &stagingMemory,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vkBindBufferMemory(base.device, stagingBuffer, stagingMemory, 0);
	void* pMem;
	vkMapMemory(base.device, stagingMemory, 0, vertexDataSize + indexDataSize, 0, &pMem);

		//copy each model's data to staging memory
	size_t currentVertexOffset = 0;
	size_t currentIndexOffset = vertexDataSize;

	stageLoadGroup(pMem, &loaded2D, currentVertexOffset, currentIndexOffset);
	stageLoadGroup(pMem, &loaded3D, currentVertexOffset, currentIndexOffset);
	stageLoadGroup(pMem, &loadedAnim3D, currentVertexOffset, currentIndexOffset);

#ifndef NDEBUG
	std::cout << "finished staging model groups" << std::endl;
#endif

	//create final dest memory
	vkhelper::createBufferAndMemory(base, vertexDataSize + indexDataSize, &buffer, &memory,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
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

#ifndef NDEBUG
	std::cout << "finished loading model data to gpu" << std::endl;
#endif
}


} //end namespace
