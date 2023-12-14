#include "descriptors.h"

#include "../vkhelper.h"
#include <stdexcept>

namespace part
{
namespace create
{

void _createDescriptorPool(VkDevice device, VkDescriptorPool* pool, std::vector<DS::DescriptorSet*> descriptorSets, uint32_t frameCount);
void _createDescriptorSet(VkDevice device, VkDescriptorPool pool, DS::DescriptorSet *ds, uint32_t frameCount);
size_t _createHostVisibleShaderBufferMemory(DeviceState base, std::vector<DS::Binding*> ds, VkBuffer* buffer, VkDeviceMemory* memory);

void DescriptorSetLayout(VkDevice device, DS::DescriptorSet *ds, std::vector<DS::Binding*> bindings, VkShaderStageFlagBits stageFlags)
{
	//create layout
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings(bindings.size());
	ds->poolSize.resize(bindings.size());
	for(size_t i = 0; i < bindings.size(); i++)
	{
		bindings[i]->binding = static_cast<uint32_t>(i);
		layoutBindings[i].binding = static_cast<uint32_t>(bindings[i]->binding);
		layoutBindings[i].descriptorType = bindings[i]->type;
		layoutBindings[i].descriptorCount = static_cast<uint32_t>(bindings[i]->descriptorCount);
		layoutBindings[i].stageFlags = stageFlags;

		ds->poolSize[i].type = bindings[i]->type;
		ds->poolSize[i].descriptorCount = static_cast<uint32_t>(bindings[i]->descriptorCount);

	}

	VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	layoutInfo.pBindings = layoutBindings.data();
	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &ds->layout) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor sets");
}

void DescriptorPoolAndSet(VkDevice device, VkDescriptorPool* pool, std::vector<DS::DescriptorSet*> descriptorSets, uint32_t frameCount)
{
  _createDescriptorPool(device, pool, descriptorSets, frameCount);
  for(int i = 0; i < descriptorSets.size(); i++)
  {
    _createDescriptorSet(device, *pool, descriptorSets[i], frameCount);
  }
}

  void PrepareShaderBufferSets(DeviceState base, std::vector<DS::Binding*> bind, VkBuffer* buffer, VkDeviceMemory* memory) {
      size_t memSize = _createHostVisibleShaderBufferMemory(base, bind, buffer, memory);

      vkBindBufferMemory(base.device, *buffer, *memory, 0);
      void* pointer;
      vkMapMemory(base.device, *memory, 0, memSize, 0, &pointer);

      for (size_t bindingI = 0; bindingI < bind.size(); bindingI++){
	  bind[bindingI]->pBuffer = nullptr;

	  std::vector<VkWriteDescriptorSet> writes(bind[bindingI]->setCount, {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET});
	  std::vector<VkDescriptorBufferInfo> buffInfos;
	  std::vector<VkDescriptorImageInfo> imageInfos;
	  if( bind[bindingI]->type ==  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
	      bind[bindingI]->type == 	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
	      bind[bindingI]->type == 	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
	      bind[bindingI]->type == 	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
	      )
	      {
		  buffInfos.resize(bind[bindingI]->setCount * bind[bindingI]->descriptorCount);
		  bind[bindingI]->pBuffer = pointer;
	      }
	  if(bind[bindingI]->type == VK_DESCRIPTOR_TYPE_SAMPLER ||
	     bind[bindingI]->type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
	      imageInfos.resize(bind[bindingI]->setCount * bind[bindingI]->descriptorCount);

	  size_t buffIndex = 0;
	  for (size_t i = 0; i < bind[bindingI]->setCount; i++) {
	      writes[i].dstSet = bind[bindingI]->ds->sets[i];
	      writes[i].dstBinding = static_cast<uint32_t>(bind[bindingI]->binding);
	      writes[i].dstArrayElement = 0;
	      writes[i].descriptorCount = static_cast<uint32_t>(bind[bindingI]->descriptorCount);
	      writes[i].descriptorType = bind[bindingI]->type;
	      switch(bind[bindingI]->type) {
	      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
	      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
	      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
	      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		  for (size_t j = 0; j < bind[bindingI]->descriptorCount; j++) {
		      buffInfos[buffIndex].buffer = *buffer;
		      buffInfos[buffIndex].offset = bind[bindingI]->offset +
			  (bind[bindingI]->bufferSize * i) +
			  (bind[bindingI]->arraySize * bind[bindingI]->slotSize * j);
		      buffInfos[buffIndex].range = bind[bindingI]->slotSize * bind[bindingI]->arraySize;
		      buffIndex++;
		  }
		  writes[i].pBufferInfo = buffInfos.data() + (i * bind[bindingI]->descriptorCount);
		  break;
	      case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		  for (size_t j = 0; j < bind[bindingI]->descriptorCount; j++) {
		      size_t imageIndex = (bind[bindingI]->descriptorCount * i) + j;
		      imageInfos[imageIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		      //each set has different image views (eg per frame)
		      if(bind[bindingI]->viewsPerSet)
			  imageInfos[imageIndex].imageView = *(bind[bindingI]->imageViews +
							       (i * bind[bindingI]->descriptorCount)
							       + j);
		      else
			  imageInfos[imageIndex].imageView = *(bind[bindingI]->imageViews + j);
		  }
		  writes[i].pImageInfo = imageInfos.data() + (i * bind[bindingI]->descriptorCount);
		  break;
	      case VK_DESCRIPTOR_TYPE_SAMPLER:
		  for (size_t j = 0; j < bind[bindingI]->descriptorCount; j++) {
		      size_t imageIndex = (bind[bindingI]->descriptorCount * i) + j;
		      imageInfos[imageIndex].sampler = *(bind[bindingI]->samplers + j);
		  }
		  writes[i].pImageInfo = imageInfos.data() + (i * bind[bindingI]->descriptorCount);
		  break;
	      default:
		  throw std::runtime_error("descriptor type not recognized, in prepare shader buffer sets");
	      }
	  }
	  vkUpdateDescriptorSets(base.device,
				 (uint32_t)writes.size(),
				 writes.data(), 0, nullptr);
      }
  }

void _createDescriptorPool(VkDevice device, VkDescriptorPool* pool, std::vector<DS::DescriptorSet*> descriptorSets, uint32_t frameCount)
{
  std::vector<VkDescriptorPoolSize> poolSizes;

  for(size_t i = 0; i < descriptorSets.size(); i++)
  {
    for(size_t j = 0; j < descriptorSets[i]->poolSize.size(); j++)
    {
	poolSizes.push_back(descriptorSets[i]->poolSize[j]);
	poolSizes.back().descriptorCount *= frameCount;
    }
  }
  VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = frameCount * static_cast<uint32_t>(descriptorSets.size());
  if(vkCreateDescriptorPool(device, &poolInfo, nullptr, pool) != VK_SUCCESS)
    throw std::runtime_error("Failed to create descriptor pool!");
}

void _createDescriptorSet(VkDevice device, VkDescriptorPool pool, DS::DescriptorSet *ds, uint32_t frameCount)
{
     std::vector<VkDescriptorSetLayout> layouts(frameCount, ds->layout);
	VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
	allocInfo.pSetLayouts = layouts.data();
	ds->sets.resize(frameCount);
	if (vkAllocateDescriptorSets(device, &allocInfo, ds->sets.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate descriptor sets");
}

size_t _createHostVisibleShaderBufferMemory(DeviceState base, std::vector<DS::Binding*> ds, VkBuffer* buffer, VkDeviceMemory* memory)
{
	VkPhysicalDeviceProperties physDevProps;
	vkGetPhysicalDeviceProperties(base.physicalDevice, &physDevProps);

	size_t memorySize = 0;
	for (size_t i = 0; i < ds.size(); i++)
	{
		if(ds[i]->type != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER &&
		   ds[i]->type != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC &&
		   ds[i]->type != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER &&
		   ds[i]->type != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
		{
			continue;
		}

		VkDeviceSize alignment;
		if(ds[i]->type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
		   ds[i]->type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
			alignment = physDevProps.limits.minStorageBufferOffsetAlignment;
		else
			alignment = physDevProps.limits.minUniformBufferOffsetAlignment;

		ds[i]->slotSize = vkhelper::correctMemoryAlignment(ds[i]->dataTypeSize, alignment);
		memorySize = vkhelper::correctMemoryAlignment(memorySize, alignment);

		ds[i]->offset = memorySize;
		ds[i]->bufferSize = ds[i]->slotSize * ds[i]->descriptorCount * ds[i]->arraySize;
		memorySize += ds[i]->bufferSize * ds[i]->dynamicBufferCount * ds[i]->setCount;
	}

	vkhelper::createBufferAndMemory(base, memorySize, buffer, memory,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	return memorySize;
}


}
}
