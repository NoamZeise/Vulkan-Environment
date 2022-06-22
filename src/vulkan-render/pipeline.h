#ifndef PIPELINE_H
#define PIPELINE_H

#include "vulkan/vulkan_core.h"
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>

#include <stdint.h>
#include <vector>
#include "descriptor_structs.h"

class Pipeline
{
public:
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout layout;
	VkPipeline pipeline;
	std::vector<DS::DescriptorSet*> descriptorSets;

	void begin(VkCommandBuffer cmdBuff, size_t frameIndex)
	{
		//bind non dynamic descriptor sets
		for (size_t i = 0; i < descriptorSets.size(); i++)
			if(!descriptorSets[i]->dynamicBuffer)
				vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, layout,
									 static_cast<uint32_t>(i), 1,
									&descriptorSets[i]->sets[frameIndex],
									0, nullptr
            );
		//bind graphics pipeline
		vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	}

	void bindDynamicDS(VkCommandBuffer cmdBuff, DS::DescriptorSet *ds, size_t frameIndex, uint32_t dynOffset)
	{
		for (size_t i = 0; i < descriptorSets.size(); i++)
			if(descriptorSets[i]->dynamicBuffer)
				if(descriptorSets[i] == ds)
					vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, layout,
									static_cast<uint32_t>(i), 1,
									&descriptorSets[i]->sets[frameIndex],
									1, &dynOffset
            );
	}

	void destroy(VkDevice device)
	{
		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineLayout(device, layout, nullptr);
	}
};

struct fragPushConstants
{
	glm::vec4 colour;
	glm::vec4 texOffset;
	uint32_t TexID;
};

struct Vertex2D
{
	glm::vec3 Position;
	glm::vec2 TexCoord;

	static std::vector<VkVertexInputBindingDescription> bindingDescriptions()
	{
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex2D);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescriptions;
	}

	static std::vector<VkVertexInputAttributeDescription> attributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

		//position
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex2D, Position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex2D, TexCoord);

		return attributeDescriptions;
	}
};

struct Vertex3D
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoord;

	static std::vector<VkVertexInputBindingDescription> bindingDescriptions()
	{
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex3D);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescriptions;
	}

	static std::vector<VkVertexInputAttributeDescription> attributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

		//position
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex3D, Position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex3D, Normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex3D, TexCoord);

		return attributeDescriptions;
	}
};

struct VertexAnim3D
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoord;
	glm::ivec4 BoneIDs;
	glm::vec4  Weights;

	static std::vector<VkVertexInputBindingDescription> bindingDescriptions()
	{
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(VertexAnim3D);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescriptions;
	}

	static std::vector<VkVertexInputAttributeDescription> attributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(5);

		//position
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(VertexAnim3D, Position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(VertexAnim3D, Normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(VertexAnim3D, TexCoord);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SINT;
		attributeDescriptions[3].offset = offsetof(VertexAnim3D, BoneIDs);

		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[4].offset = offsetof(VertexAnim3D, Weights);

		return attributeDescriptions;
	}
};


#endif
