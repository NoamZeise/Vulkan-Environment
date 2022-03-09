#include "model_loader.h"

namespace Resource
{

ModelLoader::ModelLoader(Base base, VkCommandPool pool)
{
	this->base = base;
	this->pool = pool;

	loadQuad();
}

ModelLoader::~ModelLoader()
{
	if(models.size() <= 0)
		return;
	for (auto& model : loadedModels)
			for (size_t i = 0; i < model.meshes.size(); i++)
				delete model.meshes[i];

	vkDestroyBuffer(base.device, buffer, nullptr);
	vkFreeMemory(base.device, memory, nullptr);
}

void ModelLoader::bindBuffers(VkCommandBuffer cmdBuff)
{
	if(currentIndex == 0)
		return;
	VkBuffer vertexBuffers[] = { buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmdBuff, 0, 1, vertexBuffers, offsets);
	//bind index buffer - can only have one index buffer
	vkCmdBindIndexBuffer(cmdBuff, buffer, vertexDataSize, VK_INDEX_TYPE_UINT32);
}

void ModelLoader::drawModel(VkCommandBuffer cmdBuff, VkPipelineLayout layout, Model model, size_t count, size_t instanceOffset)
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

void ModelLoader::drawQuad(VkCommandBuffer cmdBuff, VkPipelineLayout layout, unsigned int texID, size_t count, size_t instanceOffset, glm::vec4 colour, glm::vec4 texOffset)
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

void ModelLoader::loadQuad()
{
	currentIndex++;
	LoadedModel ldModel;
	ldModel.directory = "quad";
	ldModel.meshes.push_back(new Mesh());
	ldModel.meshes.back()->texture =  Texture(0, glm::vec2(0,0), "quad");
	ldModel.meshes.back()->verticies =
		{
			//pos   			normal  			texcoord
			{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
			{{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
			{{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
			{{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
		};
	ldModel.meshes.back()->indicies = { 0, 3, 2, 2, 1, 0};

	loadedModels.push_back(ldModel);
}

Model ModelLoader::loadModel(std::string path, TextureLoader &texLoader)
{
#ifndef NO_ASSIMP
	Model model(currentIndex++);
	Assimp::Importer importer;
	const aiScene *scene = importer.ReadFile(path,
		aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_FlipUVs |
		aiProcess_JoinIdenticalVertices | aiProcess_GenNormals);
	if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		throw std::runtime_error("failed to load model at \"" + path + "\" assimp error: " + importer.GetErrorString());


	LoadedModel ldModel;
	ldModel.directory = path.substr(0, path.find_last_of('/'));

	//correct for blender's orientation
	glm::mat4 transform = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	transform = glm::scale(transform, glm::vec3(0.02f));
	aiMatrix4x4 aiTransform = aiMatrix4x4(
		transform[0][0], transform[0][1], transform[0][2], transform[0][3],
		transform[1][0], transform[1][1], transform[1][2], transform[1][3],
		transform[2][0], transform[2][1], transform[2][2], transform[2][3],
		transform[3][0], transform[3][1], transform[3][2], transform[3][3]);
	processNode(&ldModel, scene->mRootNode, scene, texLoader, aiTransform);

	loadedModels.push_back(ldModel);

	return model;
#else
	throw std::runtime_error("tried to load model but NO_ASSIMP is defined!");
#endif
}

#ifndef NO_ASSIMP
void ModelLoader::processNode(LoadedModel* model, aiNode* node, const aiScene* scene, TextureLoader &texLoader, aiMatrix4x4 parentTransform)
{
	aiMatrix4x4 transform = parentTransform * node->mTransformation;
	for(unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
		model->meshes.push_back(new Mesh());
		processMesh(model->meshes.back(), mesh, scene, texLoader, transform);
	}
	for(unsigned int i = 0; i < node->mNumChildren; i++, texLoader)
	{
		processNode(model, node->mChildren[i], scene, texLoader, transform);
	}
}
void ModelLoader::processMesh(Mesh* mesh, aiMesh* aimesh, const aiScene* scene, TextureLoader &texLoader, aiMatrix4x4 transform)
{
	loadMaterials(mesh, scene->mMaterials[aimesh->mMaterialIndex], texLoader);

	//vertcies
	for(unsigned int i = 0; i < aimesh->mNumVertices;i++)
	{
		aiVector3D transformedVertex = transform * aimesh->mVertices[i];
		Vertex vertex;
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
	//indicies
	for(unsigned int i = 0; i < aimesh->mNumFaces; i++)
	{
		aiFace face = aimesh->mFaces[i];
		for(unsigned int j = 0; j < face.mNumIndices; j++)
			mesh->indicies.push_back(face.mIndices[j]);
	}
}
void ModelLoader::loadMaterials(Mesh* mesh, aiMaterial* material, TextureLoader &texLoader)
{
	for(unsigned int i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); i++)
	{
		aiString aistring;
		material->GetTexture(aiTextureType_DIFFUSE, i, &aistring);
		std::string texLocation = aistring.C_Str();
		texLocation = MODEL_TEXTURE_LOCATION + texLocation;

		bool skip = false;
		for(unsigned int j = 0; j < alreadyLoaded.size(); j++)
		{
			if(std::strcmp(alreadyLoaded[j].path.data(), texLocation.c_str()) == 0)
			{
				mesh->texture = alreadyLoaded[j];
				skip = true;
				break;
			}
		}
		if(!skip)
		{
			mesh->texture = texLoader.loadTexture(texLocation);
			mesh->texture.type = TextureType::Diffuse; //attention
			alreadyLoaded.push_back(mesh->texture);
		}
	}
}
#endif

void ModelLoader::endLoading(VkCommandBuffer transferBuff)
{
	if(loadedModels.size() == 0)
		return;

	//load to staging buffer

	for(size_t i = 0; i < loadedModels.size(); i++)
	{
		ModelInGPU model;

		model.vertexOffset = vertexDataSize / sizeof(loadedModels[i].meshes[0]->verticies[0]);
		model.indexOffset = indexDataSize / sizeof(loadedModels[i].meshes[0]->indicies[0]);
		model.meshes.resize(loadedModels[i].meshes.size());
		for(size_t j = 0 ; j <  loadedModels[i].meshes.size(); j++)
		{
			model.meshes[j] = MeshInfo(
					loadedModels[i].meshes[j]->indicies.size(),
					model.indexCount,
					model.vertexCount,
					loadedModels[i].meshes[j]->texture);
			model.vertexCount += loadedModels[i].meshes[j]->verticies.size();
			model.indexCount  += loadedModels[i].meshes[j]->indicies.size();
			vertexDataSize += sizeof(loadedModels[i].meshes[j]->verticies[0]) * loadedModels[i].meshes[j]->verticies.size();
			indexDataSize +=  sizeof(loadedModels[i].meshes[j]->indicies[0]) * loadedModels[i].meshes[j]->indicies.size();
		}
		models.push_back(model);
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
