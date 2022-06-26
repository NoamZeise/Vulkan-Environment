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
	Model loadModel(std::string path, TextureLoader* texLoader, std::vector<Resource::ModelAnimation> *pGetAnimations);
	void endLoading(VkCommandBuffer transferBuff);

	void bindBuffers(VkCommandBuffer cmdBuff);
	void drawModel(VkCommandBuffer cmdBuff, VkPipelineLayout layout, Model model, size_t count, size_t instanceOffset);
	void drawQuad(VkCommandBuffer cmdBuff, VkPipelineLayout layout, unsigned int texID, size_t count, size_t instanceOffset, glm::vec4 colour, glm::vec4 texOffset);

	int getAnimationIndex(Model model, std::string animationName);
	ModelAnimation* getpAnimation(Model model, int animationIndex);

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
		glm::vec4 diffuseColour;
	};

	template <class T_Vert>
	struct LoadedModel
	{
		LoadedModel(){}
		int ID = -1;
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
		MeshInfo(size_t indexCount, size_t indexOffset, size_t vertexOffset, Texture texture, glm::vec4 diffuseColour)
		{
			this->indexCount = indexCount;
			this->indexOffset = indexOffset;
			this->vertexOffset = vertexOffset;
			this->texture = texture;
			this->diffuseColour = diffuseColour;
		}
		size_t indexCount;
		size_t indexOffset;
		size_t vertexOffset;
		Texture texture;
		glm::vec4 diffuseColour;
	};

	struct ModelInGPU
	{
		size_t vertexCount = 0;
		size_t indexCount  = 0;
		size_t vertexOffset = 0;
		size_t indexOffset = 0;
		std::vector<MeshInfo> meshes;

		std::vector<ModelAnimation> animations;
		std::map<std::string, int> animationMap;
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
	std::map<int, ModelInGPU> models;
	VkBuffer buffer;
	VkDeviceMemory memory;

	size_t vertexDataSize = 0;

	size_t indexDataSize = 0;

	size_t currentIndex = 0;

	bool boundThisFrame = false;
	ModelType prevBoundType;

	size_t quadID = 0;
};

}


#endif
