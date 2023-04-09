#ifndef VK_ENV_SHADER_H
#define VK_ENV_SHADER_H

// TO FIX FROM OLD
// - desciptor set keeps track of if dynamic, for pipeline binding
// - binding needed reference to parent set, for PrepareShaderbuffersets fn
// - binding needed setCount (number of duplicates for a set (ie per frames))
// for Prepareshaderbuffersets fn
// - binding needed viewsPerSet boolean, ie. perFrameImageViews(ie sampling colour attachment in final) vs image views(ie texture views - doesnt change per frame) for Prepareshaderbuffersets.


#include <vector>
#include <string>

/// Specify Shader Descriptors



namespace descriptor {

  enum class ShaderStage {
      Vertex,
      Fragment,
  };

  enum class DescriptorType {
      UniformBuffer,
      UniformBufferDynamic,
      StorageBuffer,
      StorageBufferDynamic,
      Sampler,
      SampledImage,
  };

  struct Descriptor {
      std::string name;
      DescriptorType type;
      size_t dataTypeSize;
      // single struct array member size or array of descriptors size or sampler/view count
      size_t dataArraySize;
      size_t dynamicBufferSize;
      bool isSingleArrayStruct = false;
      void* pSamplerOrImageViews;

      Descriptor(std::string name, DescriptorType type, size_t typeSize,
		 size_t dataArraySize, size_t dynamicSize, void* pSamplerOrImageViews);
      Descriptor(std::string name, DescriptorType type, size_t typeSize, size_t arraySize);

  private:
      void init(std::string name, DescriptorType type, size_t typeSize,
		 size_t dataArraySize, size_t dynamicSize, void* pSamplerOrImageViews);
  };

  
  struct Set {
      std::vector<Descriptor> descriptors;
      std::string name;
      ShaderStage shaderStage;
  
      Set(std::string name, ShaderStage shaderStage);
      Set(){};
      /// adding order matters, must match shader
      void AddDescriptor(Descriptor descriptor);
      void AddDescriptor(std::string name, DescriptorType type, size_t typeSize, size_t arraySize);
      void AddSingleArrayStructDescriptor(std::string name, DescriptorType type, size_t typeSize, size_t arraySize);
      void AddDescriptorDynamicWithArr(std::string name, DescriptorType type,
				       size_t typeSize, size_t arraySize, size_t dynamicSize);
      void AddSamplerDescriptor(std::string name, size_t samplerCount, void* pSamplers);
      void AddImageViewDescriptor(std::string name, size_t viewCount, void* pImageViews);

      void logDetails();
  };

}

#endif
