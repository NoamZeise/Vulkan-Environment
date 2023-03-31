#include "shader_internal.h"

#include "descriptor_structs.h"

#include <vector>


DescSet::DescSet(descriptor::Set set, size_t frameCount, VkDevice device) {
  std::vector<DS::Binding> bindings;
  for(auto& desc: set.descriptors) {
    DS::Binding binding;
    binding.ds = &(this->set);
    if(desc.isSingleStructArray) {
      binding.arraySize = desc.dataArraySize;
    } else {
      binding.descriptorCount = desc.dataArraySize;
    }
    binding.dataTypeSize = desc.dataTypeSize;
    switch(desc.type) {
      
    }
  }
}
