#include "shader.h"
#include "logger.h"

namespace descriptor {

  Set::Set(std::string name, ShaderStage shaderStage) {
    this->name = name;
    this->shaderStage = shaderStage;
  }
  
  Descriptor::Descriptor(std::string name, Type type,
			 size_t typeSize, size_t dataArraySize, size_t dynamicSize,
			 void* pSamplerOrImageViews) {
      init(name, type, typeSize, dataArraySize, dynamicSize, pSamplerOrImageViews);
  }

  Descriptor::Descriptor(std::string name, Type type, size_t typeSize, size_t arraySize) {
      size_t arrSizeVal = 1;
      size_t dynSizeVal = 1;
      if(type == Type::UniformBufferDynamic ||
	 type == Type::StorageBufferDynamic) {
	  dynSizeVal = arraySize;
      } else {
	  arrSizeVal = arraySize;
      }
      init(name, type, typeSize, arrSizeVal, dynSizeVal, nullptr);
  }

  void Descriptor::init(std::string name, Type type, size_t typeSize,
			      size_t dataArraySize, size_t dynamicSize,
			      void* pSamplerOrImageViews) {
      this->name = name;
      this->type = type;
      this->dataTypeSize = typeSize;
      this->dataArraySize = dataArraySize;
      this->dynamicBufferSize = dynamicSize;
      this->pSamplerOrImageViews = pSamplerOrImageViews;
  }

  void Set::AddDescriptor(Descriptor descriptor) {
      descriptors.push_back(descriptor);
  }
  
  void Set::AddDescriptor(std::string name, Type type,
			  size_t typeSize, size_t arraySize) {
      AddDescriptor(Descriptor(name, type, typeSize, arraySize));
  }
  
  void Set::AddSingleArrayStructDescriptor(std::string name,
					   Type type, size_t typeSize, size_t arraySize) {
    AddDescriptor(name, type, typeSize, arraySize);
    descriptors.back().isSingleArrayStruct = true;
  }

  void Set::AddDescriptorDynamicWithArr(std::string name, Type type, size_t typeSize, size_t arraySize, size_t dynamicSize) {
    if(type != Type::StorageBufferDynamic &&
       type != Type::UniformBufferDynamic) {
      LOG_ERROR("Tried to add non dynamic descriptor type with"
		"Set::AddDescriptorDynamicWithArr");
      throw std::runtime_error("failed to add descriptor to set");
    }
    this->descriptors.push_back(
	    Descriptor(name,
		       type,
		       typeSize,
		       arraySize,
		       dynamicSize,
		       nullptr));
  }

  void Set::AddSamplerDescriptor(std::string name, size_t samplerCount, void* pSamplers) {
    this->descriptors.push_back(
	    Descriptor(name,
		       Type::Sampler,
		       0,
		       samplerCount,
		       1,
		       pSamplers));
  }

  void Set::AddImageViewDescriptor(std::string name, Type type, size_t viewCount, void* pImageViews) {
      if(type != Type::SampledImage &&
	 type != Type::SampledImagePerSet) {
	  throw std::runtime_error("Tried to add image view to Descriptor Set that isn't"
				   "sampled image or sampled image set");
      }
   this->descriptors.push_back(
	   Descriptor(name,
		      type,
		      0,
		      viewCount,
		      1,
		      pImageViews));
  }

  void Set::logDetails() {
      LOG("Set name: " << this->name);
      for(int i = 0; i < this->descriptors.size(); i++) {
	  LOG("binding " << i << "\nname: " <<
	      this->descriptors[i].name);
      }
  }
}
