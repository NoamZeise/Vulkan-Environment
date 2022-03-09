#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifndef NO_ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#endif
#include <vector>
#include <array>
#include <string>
#include <stdexcept>
#include <cmath>
#include <cstring>
#include <iostream>

#include "texture_loader.h"
#include "render_structs.h"
#include "pipeline.h"
#include "vkhelper.h"

namespace Resource
{

struct Model
{
	Model() { this->ID = 1000000; }
	Model(unsigned int ID)
	{
		this->ID = ID;
	}
	unsigned int ID;
};

class ModelLoader
{
public:
	ModelLoader(Base base, VkCommandPool pool);
	ModelLoader() {}
	~ModelLoader();
	Model loadModel(std::string path, TextureLoader &texLoader);
	void endLoading(VkCommandBuffer transferBuff);

	void bindBuffers(VkCommandBuffer cmdBuff);
	void drawModel(VkCommandBuffer cmdBuff, VkPipelineLayout layout, Model model, size_t count, size_t instanceOffset);
	void drawQuad(VkCommandBuffer cmdBuff, VkPipelineLayout layout, unsigned int texID, size_t count, size_t instanceOffset, glm::vec4 colour, glm::vec4 texOffset);

private:

	struct Mesh
	{
		Mesh() {}
		std::vector<Vertex> 	    verticies;
		std::vector<unsigned int> indicies;
		Texture texture;
	};

	struct LoadedModel
	{
		LoadedModel(){}
		std::vector<Mesh*> meshes;
		std::string        directory;
	};

	struct MeshInfo
	{
		MeshInfo() { indexCount = 0; indexOffset = 0; vertexOffset = 0; }
		MeshInfo(size_t indexCount, size_t indexOffset, size_t vertexOffset, Texture texture)
		{
			this->indexCount = indexCount;
			this->indexOffset = indexOffset;
			this->vertexOffset = vertexOffset;
			this->texture = texture;
		}
		size_t indexCount;
		size_t indexOffset;
		size_t vertexOffset;
		Texture texture;
	};

	struct ModelInGPU
	{
		unsigned int vertexCount = 0;
		unsigned int indexCount  = 0;
		unsigned int vertexOffset = 0;
		unsigned int indexOffset = 0;
		std::vector<MeshInfo> meshes;
	};

	const char* MODEL_TEXTURE_LOCATION = "textures/";
#ifndef NO_ASSIMP
    void processNode(LoadedModel* model, aiNode* node, const aiScene* scene, TextureLoader &texLoader, aiMatrix4x4 parentTransform);
	void processMesh(Mesh* mesh, aiMesh* aimesh, const aiScene* scene, TextureLoader &texLoader, aiMatrix4x4 transform);
	void loadMaterials(Mesh* mesh, aiMaterial* material, TextureLoader &texLoader);
#endif
	void loadQuad();

	Base base;
	VkCommandPool pool;
	std::vector<LoadedModel> loadedModels;
	std::vector<Texture> alreadyLoaded;
	std::vector<ModelInGPU> models;
	VkBuffer buffer;
	VkDeviceMemory memory;
	unsigned int vertexDataSize = 0;
	unsigned int indexDataSize = 0;

	unsigned int currentIndex = 0;
};

}


#endif
