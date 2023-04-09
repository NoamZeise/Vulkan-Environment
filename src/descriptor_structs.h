#ifndef DESCRIPTOR_STRUCTS_H
#define DESCRIPTOR_STRUCTS_H

#include <volk.h>

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>

#include <array>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>

#ifdef __linux__
#include <cstring>
#endif

namespace DS {
namespace ShaderStructs {
struct viewProjection {
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

struct PerFrame3D {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 normalMat;
};

struct Frag2DData {
  alignas(16) glm::vec4 colour;
  alignas(16) glm::vec4 texOffset;
  alignas(4) uint32_t texID;
};


struct timeUbo {
  float time;
};
 

struct Lighting {
  Lighting()
  {
    ambient = glm::vec4(1.0f, 1.0f, 1.0f, 0.35f);
    diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 0.8f);
    specular = glm::vec4(1.0f, 1.0f, 1.0f, 8.0f);
    direction = glm::vec4(0.3f, 0.3f, -0.5f, 0.0f);
  }

  alignas(16) glm::vec4 ambient;
  alignas(16) glm::vec4 diffuse;
  alignas(16) glm::vec4 specular;
  alignas(16) glm::vec4 direction;
  alignas(16) glm::vec4 camPos;
};

const int MAX_BONES = 50;
struct Bones
{
  alignas(16) glm::mat4 mat[MAX_BONES];
};

} // namespace ShaderStructs

struct DescriptorSet {
  void destroySet(VkDevice device)
  {
    vkDestroyDescriptorSetLayout(device, layout, nullptr);
  }
  VkDescriptorSetLayout layout;
  std::vector<VkDescriptorSet> sets;
  std::vector<VkDescriptorPoolSize> poolSize;
  bool dynamicBuffer = false;
};

struct Binding {
  DescriptorSet *ds;
  VkDescriptorType type;

  size_t setCount;
  size_t dataTypeSize;
  size_t binding = 0;
  size_t descriptorCount = 1;
  size_t arraySize = 1;
  size_t dynamicBufferCount = 1;
  size_t bufferSize = 0;

  size_t offset;
  VkDeviceSize slotSize;
  void *pBuffer;
  VkImageView *imageViews;
  VkSampler *samplers;
  bool viewsPerSet = false;

  void storeSetData(size_t frameIndex, void *data, size_t descriptorIndex,
		    size_t arrayIndex, size_t dynamicOffsetIndex)
  {
    if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
        type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
        type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
        type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
      std::memcpy(
		   static_cast<char *>(pBuffer) + offset + dynamicOffsetIndex*setCount*bufferSize +
		   ((frameIndex * bufferSize) + (descriptorIndex * arraySize * slotSize) +
		    (arrayIndex * slotSize)),
		   data, dataTypeSize);
	   else
	       throw std::runtime_error("Descriptor Shader Buffer: tried to store data "
					"in non uniform or storage buffer!");
  }
};

} // namespace DS

#endif
