#include "pipeline_data.h"
#include <resource_loader/vertex_types.h>

namespace pipeline_inputs {

  namespace V2D {

    std::vector<VkVertexInputBindingDescription> bindingDescriptions() {
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex2D);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;   
	return bindingDescriptions;
    }

    std::vector<VkVertexInputAttributeDescription> attributeDescriptions() {
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
      
	//position
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex2D, Position);
	//tex coords
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex2D, TexCoord);
      
	return attributeDescriptions;
    }
  }

  namespace V3D {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions() {
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex3D);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
      
	return bindingDescriptions;
    }
  
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions() {
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
  }

  namespace VAnim3D {

    std::vector<VkVertexInputBindingDescription> bindingDescriptions() {
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(VertexAnim3D);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescriptions;
    }

    std::vector<VkVertexInputAttributeDescription> attributeDescriptions() {
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
  }
}
