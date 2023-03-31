#ifndef VKENV_SHADER_INTERNAL_H
#define VKENV_SHADER_INTERNAL_H

#include <volk.h>

#include "shader.h"
#include "descriptor_structs.h"

#include <vector>

struct DescSet {
  DS::DescriptorSet set;
  std::vector<DS::Binding> bindings;
  DescSet(descriptor::Set set, size_t frameCount, VkDevice device);
};

#endif
