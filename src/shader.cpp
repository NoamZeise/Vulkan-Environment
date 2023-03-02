#include "shader.h"

namespace descriptor {

struct Descriptor {
    ShaderStage stage;
    DescriptorType type;
    size_t dataTypeSize;
    // single struct array member size or array of descriptors size or sampler/view count
    size_t dataArraySize;
    size_t dynamicBufferSize;

    Descriptor(ShaderStage shader, DescriptorType type, size_t typeSize, size_t dataSize,
	       size_t dynamicSize) {
	this->stage = shader;
	this->type = type;
	this->dataTypeSize = typeSize;
	this->dataArraySize = dataSize;
	this->dynamicBufferSize = dynamicSize;
    }
};

  
  void Set::AddDescriptor(ShaderStage shader, DescriptorType type,
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
	      Descriptor(shader,
			 type,
			 typeSize,
			 arrSizeVal,
			 dynSizeVal));
      
  }

}
