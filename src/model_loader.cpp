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
	
}


Model ModelLoader::loadModel(std::string path)
{
	Model model(0);
	const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
	if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "failed to load model at \"" << path << "\" assimp error: " << importer.GetErrorString() << std::endl; 
	    return model;
	}
	LoadedModel ldModel;
	ldModel.directory = path.substr(0, path.find_last_of('/'));
	
	return model;
}

void ModelLoader::processNode(LoadedModel* model, aiNode* node, const aiScene* scene)
{
	for(unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
		model->meshes.push_back(Mesh());
		processMesh(&model->meshes.back(), mesh, scene);
	}
	for(unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(model, node->mChildren[i], scene);
	}
}
void ModelLoader::processMesh(Mesh* mesh, aiMesh* aimesh, const aiScene* scene)
{
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
			vertex.Normal.x = aimesh->mTextureCoords[0][i].x;
			vertex.Normal.y = aimesh->mTextureCoords[0][i].y;
		}
		else
			vertex.TexCoord = glm::vec2(0, 0);

		mesh->verticies.push_back(vertex);
	}
	//indicies
	for(unsigned int i = 0; i < aimesh->mNumFaces; i++)
	{
		aiFace face = aimesh->mFaces[i];
		for(unsigned int j = 0; j < face.mNumIndices; j++)
			mesh->indicies.push_back(face.mIndices[j]);
	}
	//textures
	aiMaterial* material = scene->mMaterials[aimesh->mMaterialIndex];
	loadMaterials(mesh, material, aiTextureType_DIFFUSE, TextureType::Diffuse);
	loadMaterials(mesh, material, aiTextureType_SPECULAR, TextureType::Specular);
	loadMaterials(mesh, material, aiTextureType_AMBIENT, TextureType::Ambient);
}

void ModelLoader::loadMaterials(Mesh* mesh, aiMaterial* material, aiTextureType type, TextureType textype)
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
			mesh->textures.push_back(texLoader->loadTexture(aistring.C_Str()));
			mesh->textures.back().type = textype;
			alreadyLoaded.push_back(mesh->textures.back());
		}
	}
}

void ModelLoader::endLoading()
{

}


//reference
/*
void loadDataToGpu()
{

	size_t vertexSize = sizeof(mQuadVerts[0]) * mQuadVerts.size();
	size_t indexSize = sizeof(mQuadInds[0]) * mQuadInds.size();

	VkBufferCreateInfo stagingBufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	stagingBufferInfo.size = vertexSize + indexSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingBufferInfo.queueFamilyIndexCount = 1;
	stagingBufferInfo.pQueueFamilyIndices = &mBase.queue.graphicsPresentFamilyIndex;
	stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(mBase.device, &stagingBufferInfo, nullptr, &mMemory.stagingBuffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create vertex buffer!");
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(mBase.device, mMemory.stagingBuffer, &memRequirements);

	uint32_t memIndex = vkhelper::findMemoryIndex(mBase.physicalDevice, memRequirements.memoryTypeBits,
		(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));

	VkMemoryAllocateInfo memInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memInfo.allocationSize = memRequirements.size;
	memInfo.memoryTypeIndex = memIndex;
	if (vkAllocateMemory(mBase.device, &memInfo, nullptr, &mMemory.stagingMemory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate memory");

	vkBindBufferMemory(mBase.device, mMemory.stagingBuffer, mMemory.stagingMemory, 0);

	void* data;
	vkMapMemory(mBase.device, mMemory.stagingMemory, 0, memRequirements.size, 0, &data);
	std::memcpy(data, mQuadVerts.data(), vertexSize);
	std::memcpy(static_cast<char*>(data) + vertexSize, mQuadInds.data(), indexSize);
}

void copyDataToLocalGPUMemory()
{
	size_t vertexSize = sizeof(mQuadVerts[0]) * mQuadVerts.size();
	size_t indexSize = sizeof(mQuadInds[0]) * mQuadInds.size();

	VkBufferCreateInfo vbufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	vbufferInfo.size = vertexSize;
	vbufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	vbufferInfo.queueFamilyIndexCount = 1;
	vbufferInfo.pQueueFamilyIndices = &mBase.queue.graphicsPresentFamilyIndex;
	vbufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(mBase.device, &vbufferInfo, nullptr, &mMemory.vertexBuffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create vertex buffer!");
	VkMemoryRequirements vmemRequirements;
	vkGetBufferMemoryRequirements(mBase.device, mMemory.vertexBuffer, &vmemRequirements);

	VkBufferCreateInfo ibufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	ibufferInfo.size = indexSize;
	ibufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	ibufferInfo.queueFamilyIndexCount = 1;
	ibufferInfo.pQueueFamilyIndices = &mBase.queue.graphicsPresentFamilyIndex;
	ibufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(mBase.device, &ibufferInfo, nullptr, &mMemory.indexBuffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create vertex buffer!");
	VkMemoryRequirements imemRequirements;
	vkGetBufferMemoryRequirements(mBase.device, mMemory.indexBuffer, &imemRequirements);

	uint32_t memIndex = vkhelper::findMemoryIndex(
		mBase.physicalDevice,imemRequirements.memoryTypeBits & vmemRequirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkMemoryAllocateInfo memInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	memInfo.allocationSize = vmemRequirements.size + imemRequirements.size;
	memInfo.memoryTypeIndex = memIndex;
	if (vkAllocateMemory(mBase.device, &memInfo, nullptr, &mMemory.memory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate memory");

	vkBindBufferMemory(mBase.device, mMemory.vertexBuffer, mMemory.memory, 0);
	//begin command recording
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(mTransferCommandBuffer, &beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = vertexSize;
	vkCmdCopyBuffer(mTransferCommandBuffer, mMemory.stagingBuffer, mMemory.vertexBuffer,1, &copyRegion);

	vkEndCommandBuffer(mTransferCommandBuffer);
	//submit commands for execution
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mTransferCommandBuffer;
	vkQueueSubmit(mBase.queue.graphicsPresentQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(mBase.queue.graphicsPresentQueue);

	vkResetCommandPool(mBase.device, mGeneralCommandPool, 0);

	vkBindBufferMemory(mBase.device, mMemory.indexBuffer, mMemory.memory, vmemRequirements.size);
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(mTransferCommandBuffer, &beginInfo);

	copyRegion.srcOffset = vertexSize;
	copyRegion.size = indexSize;
	vkCmdCopyBuffer(mTransferCommandBuffer, mMemory.stagingBuffer, mMemory.indexBuffer, 1, &copyRegion);

	vkEndCommandBuffer(mTransferCommandBuffer);
	//submit commands for execution
	submitInfo.pCommandBuffers = &mTransferCommandBuffer;
	vkQueueSubmit(mBase.queue.graphicsPresentQueue, 1, &submitInfo, VK_NULL_HANDLE);

	vkQueueWaitIdle(mBase.queue.graphicsPresentQueue);
	vkResetCommandPool(mBase.device, mGeneralCommandPool, 0);

}
*/




}