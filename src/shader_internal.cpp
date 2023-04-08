#include "shader_internal.h"

#include "descriptor_structs.h"
#include "parts/descriptors.h"

#include <vector>

DescSet::DescSet(descriptor::Set set, size_t frameCount, VkDevice device) {
  this->highLevelSet = set;
  this->device = device;

  std::vector<DS::Binding*> bindingRef;
  for(auto& desc: set.descriptors) {
    DS::Binding binding;
    binding.ds = &(this->set);
    if(desc.isSingleArrayStruct) {
      binding.arraySize = desc.dataArraySize;
    } else {
      binding.descriptorCount = desc.dataArraySize;
    }
    binding.setCount = frameCount;
    binding.dataTypeSize = desc.dataTypeSize;
    switch(desc.type) {
    case descriptor::DescriptorType::UniformBuffer:
      binding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      break;
    case descriptor::DescriptorType::UniformBufferDynamic:
      this->set.dynamicBuffer = true;
      binding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
      break;
    case descriptor::DescriptorType::StorageBuffer:
      binding.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      break;
    case descriptor::DescriptorType::StorageBufferDynamic:
      this->set.dynamicBuffer = true;
      binding.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
      break;
    case descriptor::DescriptorType::Sampler:
      binding.type = VK_DESCRIPTOR_TYPE_SAMPLER;
      //todo handle this
    case descriptor::DescriptorType::SampledImage:
      binding.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
      //todo handle this
    default:
      throw std::runtime_error("Unrecognised descriptor type in DescSet contructor");
    }
    binding.dynamicBufferCount = desc.dynamicBufferSize;
    binding.imageViews = nullptr;
    binding.samplers = nullptr;
    this->bindings.push_back(binding);
    bindingRef.push_back(&this->bindings.back());
  }
  VkShaderStageFlagBits stage;
  switch(set.shaderStage) {
  case descriptor::ShaderStage::Vertex:
    stage = VK_SHADER_STAGE_VERTEX_BIT;
    break;
  case descriptor::ShaderStage::Fragment:
    stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    break;
  default:
    throw std::runtime_error("Unrecognised shader stage in DescSet constructor");
  }
  part::create::DescriptorSetLayout(device, &this->set, bindingRef, stage);
}


DescSet::~DescSet() {
  set.destroySet(device);
}
