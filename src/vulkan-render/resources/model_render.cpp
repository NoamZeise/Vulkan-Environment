#include "model_render.h"

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
	for (auto& model : loadedModels)
			for (size_t i = 0; i < model.meshes.size(); i++)
				delete model.meshes[i];

	vkDestroyBuffer(base.device, buffer, nullptr);
	vkFreeMemory(base.device, memory, nullptr);
}

void ModelRender::bindBuffers(VkCommandBuffer cmdBuff)
{
	if(currentIndex == 0)
		return;
	VkBuffer vertexBuffers[] = { buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmdBuff, 0, 1, vertexBuffers, offsets);
	//bind index buffer - can only have one index buffer
	vkCmdBindIndexBuffer(cmdBuff, buffer, vertexDataSize, VK_INDEX_TYPE_UINT32);
}

void ModelRender::drawModel(VkCommandBuffer cmdBuff, VkPipelineLayout layout, Model model, size_t count, size_t instanceOffset)
{
	if(model.ID >= models.size())
	{
		std::cout << "the model ID is out of range, ID: " << model.ID << std::endl;
		return;
	}
	ModelInGPU *modelInfo = &models[model.ID];
	for(size_t i = 0; i < modelInfo->meshes.size(); i++)
	{
		fragPushConstants fps{
			glm::vec4(1.0f), //colour
			glm::vec4(0, 0, 1, 1), //texOffset
			modelInfo->meshes[i].texture.ID
		};

		vkCmdPushConstants(cmdBuff, layout, VK_SHADER_STAGE_FRAGMENT_BIT,
			sizeof(vectPushConstants), sizeof(fragPushConstants), &fps);

		vkCmdDrawIndexed(cmdBuff, modelInfo->meshes[i].indexCount, count,
			modelInfo->meshes[i].indexOffset + modelInfo->indexOffset,
			modelInfo->meshes[i].vertexOffset + modelInfo->vertexOffset, instanceOffset);
	}
}

void ModelRender::drawQuad(VkCommandBuffer cmdBuff, VkPipelineLayout layout, unsigned int texID, size_t count, size_t instanceOffset, glm::vec4 colour, glm::vec4 texOffset)
{
		fragPushConstants fps{
			colour,
			texOffset,
			texID
		};
		vkCmdPushConstants(cmdBuff, layout, VK_SHADER_STAGE_FRAGMENT_BIT,
			sizeof(vectPushConstants), sizeof(fragPushConstants), &fps);

		ModelInGPU *modelInfo = &models[0];
		vkCmdDrawIndexed(cmdBuff, modelInfo->meshes[0].indexCount, count,
			modelInfo->meshes[0].indexOffset + modelInfo->indexOffset,
			modelInfo->meshes[0].vertexOffset + modelInfo->vertexOffset, instanceOffset);
}

void ModelRender::loadQuad()
{
	currentIndex++;
	loadedModels.push_back(LoadedModel());
	LoadedModel* ldModel = &loadedModels[loadedModels.size() - 1];
	ldModel->directory = "quad";
	ldModel->meshes.push_back(new Mesh());
	Mesh* mesh = ldModel->meshes[ldModel->meshes.size() - 1];
	mesh->texture =  Texture(0, glm::vec2(0,0), "quad");
	mesh->verticies =
		{
			//pos   			normal  			texcoord
			{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
			{{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
			{{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
			{{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
		};
	mesh->indicies = { 0, 3, 2, 2, 1, 0};

}

Model ModelRender::loadModel(std::string path, TextureLoader* texLoader)
{
#ifndef NO_ASSIMP

#ifndef NDEBUG
	std::cout << "loading model: " << path << std::endl;
#endif

	Model model(currentIndex++);

	ModelInfo::Model loadM = modelLoader.LoadModel(path);

	loadedModels.push_back(LoadedModel());
	LoadedModel* ldModel = &loadedModels[loadedModels.size() - 1];
	ldModel->directory = path.substr(0, path.find_last_of('/'));

	for(int mesh = 0; mesh < loadM.meshes.size(); mesh++)
	{
		ldModel->meshes.push_back(new Mesh{});
		auto ldMesh = ldModel->meshes.back();

		// just take one diffuse texture for now
		// TODO support multiple textures
		if(loadM.meshes[mesh].diffuseTextures.size() > 0)
			ldMesh->texture = loadTexture(loadM.meshes[mesh].diffuseTextures[0], texLoader);

		//TODO transform animated meshes by animation per frame instead of bind pose
		glm::mat4 meshTransform = loadM.correction * loadM.meshes[mesh].bindTransform;
		for(int vert = 0; vert < loadM.meshes[mesh].verticies.size(); vert++)
		{
			Vertex vertex;
			vertex.Position = meshTransform * glm::vec4(loadM.meshes[mesh].verticies[vert].Position, 1.0f);
			vertex.Normal = loadM.meshes[mesh].verticies[vert].Normal;
			vertex.TexCoord = loadM.meshes[mesh].verticies[vert].TexCoord;
			ldMesh->verticies.push_back(vertex);
		}

		ldMesh->indicies = loadM.meshes[mesh].indicies;
	}

	return model;
#else
	throw std::runtime_error("tried to load model but NO_ASSIMP is defined!");
#endif
}

#ifndef NO_ASSIMP

Resource::Texture ModelRender::loadTexture(std::string path, TextureLoader* texLoader)
{
	std::string texLocation = MODEL_TEXTURE_LOCATION + path;

	for(unsigned int i = 0; i < alreadyLoaded.size(); i++)
		if(std::strcmp(alreadyLoaded[i].path.data(), texLocation.c_str()) == 0)
			return alreadyLoaded[i];

	alreadyLoaded.push_back(texLoader->LoadTexture(texLocation));
#ifndef NDEBUG
	std::cout << "^ for model" << std::endl;
#endif
	alreadyLoaded[alreadyLoaded.size() - 1].type = TextureType::Diffuse; //attention
	return alreadyLoaded[alreadyLoaded.size() - 1];
}
#endif

void ModelRender::endLoading(VkCommandBuffer transferBuff)
{
	if(loadedModels.size() == 0)
		return;

	//load to staging buffer

	for(size_t i = 0; i < loadedModels.size(); i++)
	{
		models.push_back(ModelInGPU());
		ModelInGPU* model = &models[models.size() - 1];

		model->vertexOffset = vertexDataSize / sizeof(loadedModels[i].meshes[0]->verticies[0]);
		model->indexOffset = indexDataSize / sizeof(loadedModels[i].meshes[0]->indicies[0]);
		model->meshes.resize(loadedModels[i].meshes.size());
		for(size_t j = 0 ; j <  loadedModels[i].meshes.size(); j++)
		{
			model->meshes[j] = MeshInfo(
					loadedModels[i].meshes[j]->indicies.size(),
					model->indexCount,
					model->vertexCount,
					loadedModels[i].meshes[j]->texture);
			model->vertexCount += loadedModels[i].meshes[j]->verticies.size();
			model->indexCount  += loadedModels[i].meshes[j]->indicies.size();
			vertexDataSize += sizeof(loadedModels[i].meshes[j]->verticies[0]) * loadedModels[i].meshes[j]->verticies.size();
			indexDataSize +=  sizeof(loadedModels[i].meshes[j]->indicies[0]) * loadedModels[i].meshes[j]->indicies.size();
		}
	}

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
	for(auto& model: loadedModels)
	{
		for(size_t i = 0; i < model.meshes.size(); i++)
		{
			std::memcpy(static_cast<char*>(pMem) + currentVertexOffset, model.meshes[i]->verticies.data(), sizeof(model.meshes[i]->verticies[0]) * model.meshes[i]->verticies.size());
			currentVertexOffset += sizeof(model.meshes[i]->verticies[0]) * model.meshes[i]->verticies.size();
			std::memcpy(static_cast<char*>(pMem) + currentIndexOffset, model.meshes[i]->indicies.data(), sizeof(model.meshes[i]->indicies[0]) * model.meshes[i]->indicies.size());
			currentIndexOffset += sizeof(model.meshes[i]->indicies[0]) * model.meshes[i]->indicies.size();
			delete model.meshes[i];
		}
	}
	loadedModels.clear();

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
}


} //end namespace
