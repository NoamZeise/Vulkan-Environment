#ifndef VKENV_SHADER_INTERNAL_H
#define VKENV_SHADER_INTERNAL_H

#include <volk.h>

#include "shader.h"
#include "descriptor_structs.h"

#include <vector>

struct DescSet {
  DescSet(descriptor::Set set, size_t frameCount, VkDevice device);
  ~DescSet();
  descriptor::Set highLevelSet;
  DS::DescriptorSet set;
  std::vector<DS::Binding> bindings;
  VkDevice device;
};

#endif
