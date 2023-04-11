#ifndef VKENV_PIPELINE_DATA_H
#define VKENV_PIPELINE_DATA_H

#include <glm/glm.hpp>
#include <volk.h>
#include <vertex_types.h>
#include <vector>

struct fragPushConstants {
    glm::vec4 colour;
    glm::vec4 texOffset;
    uint32_t TexID;
};

namespace pipeline_inputs {

  namespace V2D {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions();
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions();
  }

  namespace V3D {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions();
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions();
  }

  namespace VAnim3D {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions();
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions();
  }
}
#endif
