#include "model_loader.h"

namespace Resource
{

ModelLoader::ModelLoader(Base base, VkCommandPool pool)
{
	this->base = base;
	this->pool = pool;

}

ModelLoader::~ModelLoader()
{
	if (currentIndex != 0)
	{
		for (auto& model : loadedModels)
			for (size_t i = 0; i < model.meshes.size(); i++)
				delete model.meshes[i];

		vkDestroyBuffer(base.device, buffer, nullptr);
		vkFreeMemory(base.device, memory, nullptr);
	}
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

void ModelLoader::drawModel(VkCommandBuffer cmdBuff, Model model)
{
	if(model.ID > models.size())
	{	
		std::cout << "the model ID is out of range" << std::endl;
		return;
	}
	ModelInGPU modelInfo = models[model.ID];
	vkCmdDrawIndexed(cmdBuff, modelInfo.indexCount, 1,  modelInfo.indexOffset, modelInfo.vertexOffset, 0);
}

Model ModelLoader::loadModel(std::string path, TextureLoader &texLoader)
{
	Model model(currentIndex++);
	Assimp::Importer importer;
	const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
	if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		currentIndex--;
		std::cout << "failed to load model at \"" << path << "\" assimp error: " << importer.GetErrorString() << std::endl;
	    return model;
	}
	LoadedModel ldModel;
	ldModel.directory = path.substr(0, path.find_last_of('/'));

	processNode(&ldModel, scene->mRootNode, scene, texLoader);

	loadedModels.push_back(ldModel);

	return model;
}

void ModelLoader::processNode(LoadedModel* model, aiNode* node, const aiScene* scene, TextureLoader &texLoader)
{
	for(unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
		model->meshes.push_back(new Mesh());
		processMesh(model->meshes.back(), mesh, scene, texLoader);
	}
	for(unsigned int i = 0; i < node->mNumChildren; i++, texLoader)
	{
		processNode(model, node->mChildren[i], scene, texLoader);
	}
}
void ModelLoader::processMesh(Mesh* mesh, aiMesh* aimesh, const aiScene* scene, TextureLoader &texLoader)
{
	//textures
	if(aimesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[aimesh->mMaterialIndex];
		loadMaterials(mesh, material, aiTextureType_DIFFUSE, TextureType::Diffuse, texLoader);
		loadMaterials(mesh, material, aiTextureType_SPECULAR, TextureType::Specular, texLoader);
		loadMaterials(mesh, material, aiTextureType_AMBIENT, TextureType::Ambient, texLoader);
	}

	//vertcies
	for(unsigned int i = 0; i < aimesh->mNumVertices;i++)
	{
		Vertex vertex;
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
			vertex.Normal = glm::vec3(0, 0, 0);
		if(aimesh->mTextureCoords[0])
		{
			vertex.TexCoord.x = aimesh->mTextureCoords[0][i].x;
			vertex.TexCoord.y = aimesh->mTextureCoords[0][i].y;
			if(mesh->textures.size() > 0)
				vertex.TexCoord.z = mesh->textures[0].ID;
			else
				vertex.TexCoord.z = 0;
		}
		else
			vertex.TexCoord = glm::vec3(0, 0, 0);

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

void ModelLoader::loadMaterials(Mesh* mesh, aiMaterial* material, aiTextureType type, TextureType textype, TextureLoader &texLoader)
{
	for(unsigned int i = 0; i < material->GetTextureCount(type); i++)
	{
		aiString aistring;
		material->GetTexture(type, i, &aistring);
		bool skip = false;
		for(unsigned int j = 0; j < alreadyLoaded.size(); j++)
		{
			if(std::strcmp(alreadyLoaded[j].path.data(), aistring.C_Str()) == 0)
			{
				mesh->textures.push_back(alreadyLoaded[j]);
				skip = true;
				break;
			}
		}
		if(!skip)
		{
			mesh->textures.push_back(texLoader.loadTexture(aistring.C_Str()));
			mesh->textures.back().type = textype;
			alreadyLoaded.push_back(mesh->textures.back());
		}
	}
}

void ModelLoader::endLoading(VkCommandBuffer transferBuff)
{
	if(currentIndex == 0)
		return;

	//load to staging buffer
	for(size_t i = 0; i < loadedModels.size(); i++)
	{
		ModelInGPU model;
		for(auto& mesh: loadedModels[i].meshes)
		{
			model.vertexCount += mesh->verticies.size();
			model.indexCount  += mesh->indicies.size();
			model.vertexOffset = vertexDataSize / sizeof(mesh->verticies[0]);
			model.indexOffset = indexDataSize / sizeof(mesh->indicies[0]);
			vertexDataSize += sizeof(mesh->verticies[0]) * mesh->verticies.size();
			indexDataSize +=  sizeof(mesh->indicies[0]) * mesh->indicies.size();
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

	VkBufferCreateInfo finalbufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	finalbufferInfo.size = vertexDataSize + indexDataSize;
	finalbufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	finalbufferInfo.queueFamilyIndexCount = 1;
	finalbufferInfo.pQueueFamilyIndices = &base.queue.graphicsPresentFamilyIndex;
	finalbufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(base.device, &finalbufferInfo, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create model data buffer!");
	VkMemoryRequirements finalMemRequirements;
	vkGetBufferMemoryRequirements(base.device, buffer, &finalMemRequirements);

	uint32_t memIndex = vkhelper::findMemoryIndex(
		base.physicalDevice, finalMemRequirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkMemoryAllocateInfo memInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	memInfo.allocationSize = finalMemRequirements.size;
	memInfo.memoryTypeIndex = memIndex;
	if (vkAllocateMemory(base.device, &memInfo, nullptr, &memory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate memory");

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
