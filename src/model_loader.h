#pragma once
#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include <string>
#include <stdexcept>
#include <cmath>
#include <cstring>
#include <iostream>

#include "texture_loader.h"
#include "typeStructs.h"
#include "vkhelper.h"

namespace Resource
{

struct Model
{
	Model(unsigned int ID)
	{
		this->ID = ID;
	}
	unsigned int ID;
};

struct Mesh
{
	Mesh() {}
	std::vector<Vertex> 	    verticies;
	std::vector<unsigned int> indicies;
	std::vector<Texture>      textures;
	size_t vertexDataSize = 0;
	size_t indexDataSize = 0;
};

struct LoadedModel
{
	LoadedModel(){}
	unsigned int 			 ID = 0;
	std::vector<Mesh*> meshes;
	std::string        directory;
};

struct ModelInGPU
{
	unsigned int vertexCount = 0;
	unsigned int indexCount  = 0;
	unsigned int vertexOffset = 0;
	unsigned int indexOffset = 0;
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
	void drawModel(VkCommandBuffer cmdBuff, Model model);

private:

  void processNode(LoadedModel* model, aiNode* node, const aiScene* scene, TextureLoader &texLoader);
	void processMesh(Mesh* mesh, aiMesh* aimesh, const aiScene* scene, TextureLoader &texLoader);
	void loadMaterials(Mesh* mesh, aiMaterial* material, aiTextureType type, TextureType textype, TextureLoader &texLoader);

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
