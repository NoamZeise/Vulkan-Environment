#include "shader_internal.h"

#include "descriptor_structs.h"

#include <vector>


void processDescriptorSet(descriptor::Set set, size_t frameCount, VkDevice device) {
  std::vector<DS::Binding> bindings;

  for(auto& desc: set.descriptors) {
    DS::Binding binding;


  }
}
