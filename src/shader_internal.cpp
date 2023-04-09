#include "shader_internal.h"

#include "descriptor_structs.h"
#include "parts/descriptors.h"
#include "logger.h"

#include <vector>

DescSet::DescSet(descriptor::Set set, size_t frameCount, VkDevice device) {
  this->highLevelSet = set;
  this->device = device;

  for(auto& desc: set.descriptors) {
    DS::Binding binding;
    binding.ds = &(this->set);
    if(desc.isSingleArrayStruct) {
      binding.arraySize = desc.dataArraySize;
    } else {
      binding.descriptorCount = desc.dataArraySize;
    }
    binding.setCount = frameCount;
    binding.imageViews = nullptr;
    binding.samplers = nullptr;
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
      binding.samplers = (VkSampler*)desc.pSamplerOrImageViews;
      break;
    case descriptor::DescriptorType::SampledImagePerSet:
	binding.viewsPerSet = true;
    case descriptor::DescriptorType::SampledImage:
      binding.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
      binding.imageViews = (VkImageView*)desc.pSamplerOrImageViews;
      break;
    default:
      throw std::runtime_error("Unrecognised descriptor type in DescSet contructor");
    }
    binding.dynamicBufferCount = desc.dynamicBufferSize;
    this->bindings.push_back(binding);
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
  //until ds are moved all to new system, just pass ref as before
  std::vector<DS::Binding*> bindingRef(bindings.size());
  for(int i = 0; i < bindings.size(); i++) {
      bindingRef[i] = &bindings[i];
  }
  part::create::DescriptorSetLayout(device, &this->set, bindingRef, stage);
}


DescSet::~DescSet() { set.destroySet(device); }


void DescSet::printDetails() {
    this->highLevelSet.logDetails();
    for(int i = 0; i < this->bindings.size(); i++) {
	LOG("arraySize: " << this->bindings[i].arraySize
	    << "\ntype: " << this->bindings[i].type
	    << "\nBuffer size: " << this->bindings[i].bufferSize
	    << "\nbinding: " << this->bindings[i].binding
	    << "\nset count: " << this->bindings[i].setCount
	    << "\ndesc count: " << this->bindings[i].descriptorCount);
    }
}
