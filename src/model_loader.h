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
	std::vector<Vertex> 	  verticies;
	std::vector<unsigned int> indicies;
	std::vector<Texture>      textures;
};

struct LoadedModel
{
	LoadedModel(){}
	std::vector<Mesh> meshes;
	std::string directory;
};

class ModelLoader
{
public:
	ModelLoader(TextureLoader* texLoader) { this->texLoader = texLoader; };
	ModelLoader(Base base, VkCommandPool pool);
	~ModelLoader();
	Model loadModel(std::string path);
	void endLoading();

	VkSampler sampler;

private:

    void processNode(LoadedModel* model, aiNode* node, const aiScene* scene);
	void processMesh(Mesh* mesh, aiMesh* aimesh, const aiScene* scene);
	void loadMaterials(Mesh* mesh, aiMaterial* material, aiTextureType type, TextureType textype);

	Base base;
	VkCommandPool pool;
	Assimp::Importer importer;
	TextureLoader* texLoader;
	std::vector<LoadedModel> loadedModels;
	std::vector<Texture> alreadyLoaded;
	VkDeviceMemory memory;
};

}


#endif