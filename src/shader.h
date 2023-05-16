/// A high level description of shader buffers for pipelines.
/// Turned into the proper vulkan resources with 'shader_internal'
///
/// This has no direct dependancy on any vulkan types.

#ifndef VK_ENV_SHADER_H
#define VK_ENV_SHADER_H

#include <vector>
#include <string>

namespace descriptor {

  enum class ShaderStage {
      Vertex,
      Fragment,
  };

  enum class Type {
      UniformBuffer,
      UniformBufferDynamic,
      StorageBuffer,
      StorageBufferDynamic,
      Sampler,
      SampledImage,
      SampledImagePerSet, // i.e different samplers for each desc set
  };

  struct Descriptor {
      std::string name;
      Type type;
      size_t dataTypeSize;
      // single struct array member size or array of descriptors size or sampler/view count
      size_t dataArraySize;
      size_t dynamicBufferSize;
      bool isSingleArrayStruct = false;
      void* pSamplerOrImageViews;

      Descriptor(std::string name, Type type, size_t typeSize,
		 size_t dataArraySize, size_t dynamicSize, void* pSamplerOrImageViews);
      Descriptor(std::string name, Type type, size_t typeSize, size_t arraySize);

  private:
      void init(std::string name, Type type, size_t typeSize,
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
      void AddDescriptor(std::string name, Type type, size_t typeSize, size_t arraySize);
      void AddSingleArrayStructDescriptor(std::string name, Type type, size_t typeSize, size_t arraySize);
      void AddDescriptorDynamicWithArr(std::string name, Type type,
				       size_t typeSize, size_t arraySize, size_t dynamicSize);
      void AddSamplerDescriptor(std::string name, size_t samplerCount, void* pSamplers);
      void AddImageViewDescriptor(std::string name, Type type, size_t viewCount, void* pImageViews);

      void logDetails();
  };

}

#endif
