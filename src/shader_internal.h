#ifndef VKENV_SHADER_INTERNAL_H
#define VKENV_SHADER_INTERNAL_H

#include <volk.h>
#include "shader.h"
#include <vector>

namespace DS {
  struct DescriptorSet {
      void destroySet(VkDevice device);
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
			size_t arrayIndex, size_t dynamicOffsetIndex);
  };
}

struct DescSet {
  DescSet(descriptor::Set set, size_t frameCount, VkDevice device);
  ~DescSet();
  descriptor::Set highLevelSet;
  DS::DescriptorSet set;
  std::vector<DS::Binding> bindings;
  VkDevice device;

  void printDetails();
};

#endif
