#ifndef MODEL_RENDER_H
#define MODEL_RENDER_H

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifndef NO_ASSIMP
#include "model_loader.h"
#endif
#include <vector>
#include <array>
#include <string>
#include <stdexcept>
#include <cmath>
#include <cstring>
#include <iostream>

#include "model_animation.h"
#include "../texture_loader.h"
#include "../../render_structs.h"
#include "../../pipeline.h"
#include "../../vkhelper.h"
#include "../resources.h"

namespace Resource
{

class ModelRender
{
public:
	ModelRender(Base base, VkCommandPool pool);
	~ModelRender();
	Model loadModel(std::string path, TextureLoader* texLoader);
	void endLoading(VkCommandBuffer transferBuff);

	void bindBuffers(VkCommandBuffer cmdBuff);
	void drawModel(VkCommandBuffer cmdBuff, VkPipelineLayout layout, Model model, size_t count, size_t instanceOffset);
	void drawQuad(VkCommandBuffer cmdBuff, VkPipelineLayout layout, unsigned int texID, size_t count, size_t instanceOffset, glm::vec4 colour, glm::vec4 texOffset);

	ModelAnimation* getpAnimation(Model model, std::string animation);

private:

	enum class ModelType
	{
	    model2D,
		model3D,
		modelAnim3D,
    };

	template <class T_Vert>
	struct Mesh
	{
		Mesh() {}
		std::vector<T_Vert> verticies;
		std::vector<unsigned int> indicies;
		Texture texture;
	};

	template <class T_Vert>
	struct LoadedModel
	{
		LoadedModel(){}
		std::vector<Mesh<T_Vert>*> meshes;
		std::string        directory;
		std::vector<ModelAnimation> animations;
	};

	template <class T_Vert>
	struct ModelGroup
	{
		std::vector<LoadedModel<T_Vert>> models;
		size_t vertexDataOffset;
		size_t vertexDataSize;
	};

	ModelType getModelType(Vertex2D vert) { return ModelType::model2D; }
	ModelType getModelType(Vertex3D vert) { return ModelType::model3D; }
	ModelType getModelType(VertexAnim3D vert) { return ModelType::modelAnim3D; }

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
		std::map<std::string, ModelAnimation> animations;
		ModelType type;
	};

	const char* MODEL_TEXTURE_LOCATION = "textures/";
	void loadQuad();

	Resource::Texture loadTexture(std::string path, TextureLoader* texLoader);

	template <class T_Vert>
	void processLoadGroup(ModelGroup<T_Vert>* pGroup);

	template <class T_Vert>
	void stageLoadGroup(void* pMem, ModelGroup<T_Vert>* pGroup,
					   size_t &vertexDataOffset, size_t &indexDataOffset);

	void bindGroupVertexBuffer(VkCommandBuffer cmdBuff, ModelType type);

	ModelLoader modelLoader;

	Base base;
	VkCommandPool pool;
	ModelGroup<Vertex2D> loaded2D;
	ModelGroup<Vertex3D> loaded3D;
	ModelGroup<VertexAnim3D> loadedAnim3D;
	std::vector<Texture> alreadyLoaded;
	std::vector<ModelInGPU> models;
	VkBuffer buffer;
	VkDeviceMemory memory;

	unsigned int vertexDataSize = 0;

	unsigned int indexDataSize = 0;

	unsigned int currentIndex = 0;

	bool boundThisFrame = false;
	ModelType prevBoundType;
};

}


#endif
