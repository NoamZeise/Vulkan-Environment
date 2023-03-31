#include "shader.h"
#include "logger.h"

namespace descriptor {

    Descriptor::Descriptor(std::string name, ShaderStage shader, DescriptorType type,
			   size_t typeSize, size_t dataArraySize, size_t dynamicSize) {
      this->name = name;
      this->stage = shader;
      this->type = type;
      this->dataTypeSize = typeSize;
      this->dataArraySize = dataArraySize;
      this->dynamicBufferSize = dynamicSize;
    }

  
  void Set::AddDescriptor(std::string name, ShaderStage shader, DescriptorType type,
			  size_t typeSize, size_t arraySize) {
    size_t arrSizeVal = 1;
    size_t dynSizeVal = 1;
    if(type == DescriptorType::UniformBufferDynamic ||
       type == DescriptorType::StorageBufferDynamic) {
      dynSizeVal = arraySize;
    } else {
      arrSizeVal = arraySize;
    }
    this->descriptors.push_back(
				Descriptor(name,
					   shader,
					   type,
					   typeSize,
					   arrSizeVal,
					   dynSizeVal));
      
  }
  
  void Set::AddSingleArrayStructDescriptor(std::string name, ShaderStage shader,
					   DescriptorType type, size_t typeSize, size_t arraySize) {
    AddDescriptor(name, shader, type, typeSize, arraySize);
    descriptors.back().isSingleStructArray = true;
  }

  void Set::AddDescriptorDynamicWithArr(std::string name, ShaderStage shader, DescriptorType type, size_t typeSize, size_t arraySize, size_t dynamicSize) {
    if(type != DescriptorType::StorageBufferDynamic &&
       type != DescriptorType::UniformBufferDynamic) {
      LOG_ERROR("Tried to add non dynamic descriptor type with"
		"Set::AddDescriptorDynamicWithArr");
      throw std::runtime_error("failed to add descriptor to set");
    }
    this->descriptors.push_back(
				Descriptor(name,
					   shader,
					   type,
					   typeSize,
					   arraySize,
					   dynamicSize)
				);
  }

}
