#ifndef DESCRIPTOR_STRUCTS_H
#define DESCRIPTOR_STRUCTS_H

#include "vulkan/vulkan_core.h"

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



struct lighting {
  lighting()
  {
    ambient = glm::vec4(1.0f, 1.0f, 1.0f, 0.35f);
    diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 0.8f);
    specular = glm::vec4(1.0f, 1.0f, 1.0f, 8.0f);
    direction = glm::vec4(0.3f, -0.3f, -0.5f, 0.0f);
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
  size_t dataStructSize;
  size_t binding = 0;
  size_t descriptorCount = 1;
  size_t arraySize = 1;
  size_t dynamicBufferCount = 1;
  size_t bufferSize = 0;

  size_t offset;
  VkDeviceSize slotSize;
  void *pointer;
  VkImageView *imageViews;
  VkSampler *samplers;
  bool viewsPerSet = false;

  void storeSetData(size_t frameIndex, void *data, size_t descriptorIndex, size_t arrayIndex, size_t dynamicOffsetIndex)
  {
    if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
        type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
        type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
        type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
#ifdef _MSC_VER
      std::memcpy(
#else
           memcpy(
#endif
          static_cast<char *>(pointer) + offset + dynamicOffsetIndex*setCount*bufferSize +
              ((frameIndex * bufferSize) + (descriptorIndex * arraySize * slotSize) + (arrayIndex * slotSize)),
          data, dataStructSize);
    else
      throw std::runtime_error("Descriptor Shader Buffer: tried to store data "
                               "in non uniform or storage buffer!");
  }
};

template <typename T> struct BindingAndData
{
  Binding binding;
  std::vector<T> data;
  size_t currentDynamicOffsetIndex = 0;

  void setBufferProps(size_t setCount, VkDescriptorType type,
                      DescriptorSet *set, size_t dataCount, size_t dynamicBufferCount,
                      VkImageView *pImgViews, VkSampler *pSamplers, bool structArray) {
    data.resize(dataCount);
    binding.ds = set;
    if(structArray)
      binding.arraySize = dataCount;
    else
      binding.descriptorCount = dataCount;
    binding.setCount = setCount;
    binding.dataStructSize = sizeof(T);
    binding.type = type;
    binding.dynamicBufferCount = dynamicBufferCount;
    binding.imageViews = pImgViews;
    binding.samplers = pSamplers;

    if (binding.type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE &&
        pImgViews == nullptr)
      throw std::runtime_error("Descriptor Set Binding: type is sampled image "
                               "but pImgViews was a nullptr!");
    if (binding.type == VK_DESCRIPTOR_TYPE_SAMPLER && pSamplers == nullptr)
      throw std::runtime_error("Descriptor Set Binding: type is sampler image "
                               "but pSamplers was a nullptr!");
  }

  void setImageViewBufferProps(size_t setCount, VkDescriptorType type,
                      DescriptorSet *set, size_t dataCount, VkImageView *pImgViews) {
    setBufferProps(setCount, type, set, dataCount, 1, pImgViews, nullptr, false);
  }

    void setPerFrameImageViewBufferProps(size_t setCount, VkDescriptorType type,
                      DescriptorSet *set, VkImageView *pImgViews) {
    binding.viewsPerSet = true;
    setBufferProps(setCount, type, set, 1, 1, pImgViews, nullptr, false);
  }

  void setSamplerBufferProps(size_t setCount, VkDescriptorType type,
                      DescriptorSet *set, size_t dataCount, VkSampler *pSamplers) {
    setBufferProps(setCount, type, set, dataCount, 1, nullptr, pSamplers, false);
  }

  void setDynamicBufferProps(size_t setCount, VkDescriptorType type, DescriptorSet *set, size_t dataCount, size_t dynamicBufferCount)
  {
    if(type != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC &&
       type !=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
      throw std::runtime_error("set Dynamic Buffer, but type wasn't storage or uniform buffer dynamic");
    setBufferProps(setCount, type, set, dataCount, dynamicBufferCount, nullptr, nullptr, false);
    binding.ds->dynamicBuffer = true;
  }

  //shader data like this:
  /*
  **layout(std140, set = 0, binding = 0) uniform myData
  **{
  **    MyStruct data[10];
  **} my;
   */
  void setSingleStructArrayBufferProps(size_t setCount, VkDescriptorType type, DescriptorSet *set, size_t dataCount)
  {
    setBufferProps(setCount, type, set, dataCount, 1, nullptr, nullptr, true);
  }

  //shader data like this:
  /*layout(set = 1, binding = 0) uniform myData
  **{
  **  mat4 model;
  **  mat4 normalMatrix;
  **} data[100];
   */
  void setMultiDSBufferProps(size_t setCount, VkDescriptorType type, DescriptorSet *set, size_t dataCount)
  {
    setBufferProps(setCount, type, set, dataCount, 1, nullptr, nullptr, false);
  }

  void setBufferProps(size_t setCount, VkDescriptorType type,  DescriptorSet *set)
  {
    setBufferProps(setCount, type, set, 1, 1, nullptr, nullptr, false);
  }

  void storeData(size_t frameIndex)
  {
    for (size_t i = 0; i < binding.descriptorCount; i++)
      for(size_t j = 0; j < binding.arraySize; j++)
        storeData(frameIndex, i, j);

    if(binding.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
       binding.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
    {
      currentDynamicOffsetIndex++;
    }
  }

  void storeData(size_t frameIndex, size_t descriptorIndex, size_t arrayIndex) {
    if (arrayIndex >= data.size())
      throw std::runtime_error("Descriptor Set Binding: tried to store data in "
                               "an index outside of the data range");
    binding.storeSetData(frameIndex, &data[(descriptorIndex * binding.arraySize) + arrayIndex], descriptorIndex, arrayIndex, currentDynamicOffsetIndex);
  }
};

} // namespace DS

#endif
