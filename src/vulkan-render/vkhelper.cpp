#include "vkhelper.h"

uint32_t vkhelper::findMemoryIndex(VkPhysicalDevice physicalDevice, uint32_t memoryTypeBits, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	for (size_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if (memoryTypeBits & (1 << i)
			&& memProperties.memoryTypes[i].propertyFlags & properties)
		{
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type");
}

glm::mat4 vkhelper::getModelMatrix(glm::vec4 drawRect, float rotate)
{
	glm::mat4 model = glm::mat4(1.0f);

	model = glm::translate(model, glm::vec3(drawRect.x, drawRect.y, 0.0f)); //translate object by position
	//rotate object
	model = glm::translate(model, glm::vec3(0.5 * drawRect.z, 0.5 * drawRect.w, 0.0)); // move object by half its size, so rotates around centre
	model = glm::rotate(model, glm::radians(rotate), glm::vec3(0.0, 0.0, 1.0));//then do rotation
	model = glm::translate(model, glm::vec3(-0.5 * drawRect.z, -0.5 * drawRect.w, 0.0)); //then translate back to original position

	model = glm::scale(model, glm::vec3(drawRect.z, drawRect.w, 1.0f)); //then scale

	return model;
}

glm::vec4 vkhelper::getTextureOffset(glm::vec4 drawArea, glm::vec4 textureArea)
{
	if (drawArea.z == textureArea.z && textureArea.x == 0 && textureArea.y == 0)
		return glm::vec4(0, 0, 1, 1);

	glm::vec4 offset = glm::vec4(0, 0, 1, 1);
	offset.x = -(textureArea.x) / drawArea.z;
	offset.y = -(textureArea.y) / drawArea.w;
	offset.z = drawArea.z / textureArea.z;
	offset.w = drawArea.w / textureArea.w;

	return offset;
}


void vkhelper::createBufferAndMemory(Base base, VkDeviceSize size, VkBuffer* buffer, VkDeviceMemory* memory,
	VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.queueFamilyIndexCount = 1;
	bufferInfo.pQueueFamilyIndices = &base.queue.graphicsPresentFamilyIndex;

	if (vkCreateBuffer(base.device, &bufferInfo, nullptr, buffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create buffer of size " + size);

	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(base.device, *buffer, &memReq);

	createMemory(base.device, base.physicalDevice, memReq.size, memory, properties, memReq.memoryTypeBits);
}

void vkhelper::createMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkDeviceMemory* memory,
		VkMemoryPropertyFlags properties, uint32_t memoryTypeBits)
{
	VkMemoryAllocateInfo memInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memInfo.allocationSize = size;
	memInfo.memoryTypeIndex = findMemoryIndex(physicalDevice, memoryTypeBits, properties);

	if (vkAllocateMemory(device, &memInfo, nullptr, memory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate memory of size " + size);
}


void vkhelper::createDescriptorSet(VkDevice device, DS::DescriptorSet &ds, size_t setCount)
{
	VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	poolInfo.poolSizeCount = ds.poolSize.size();
	poolInfo.pPoolSizes = ds.poolSize.data();
	poolInfo.maxSets = setCount;
	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &ds.pool) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor pool");

	std::vector<VkDescriptorSetLayout> layouts(setCount, ds.layout);
	VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.descriptorPool = ds.pool;
	allocInfo.descriptorSetCount = layouts.size();
	allocInfo.pSetLayouts = layouts.data();
	ds.sets.resize(setCount);
	if (vkAllocateDescriptorSets(device, &allocInfo, ds.sets.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate descriptor sets");
	
}

void vkhelper::prepareShaderBufferSets(Base base,	std::vector<DS::ShaderBufferSet*> ds, 
										VkBuffer* buffer, VkDeviceMemory* memory)
{
	size_t memorySize = 0;
	for (size_t i = 0; i < ds.size(); i++)
	{
		VkDeviceSize slot = ds[i]->dsStructSize;
		VkPhysicalDeviceProperties physDevProps;
		vkGetPhysicalDeviceProperties(base.physicalDevice, &physDevProps);
		if (slot % physDevProps.limits.minUniformBufferOffsetAlignment != 0)
			slot = slot + physDevProps.limits.minUniformBufferOffsetAlignment
			- (slot % physDevProps.limits.minUniformBufferOffsetAlignment);

		ds[i]->slotSize = slot;
		ds[i]->offset = memorySize;
		memorySize += slot * ds[i]->setCount;
	}
	
	vkhelper::createBufferAndMemory(base, memorySize, buffer, memory, 
	VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
		(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));

	
	vkBindBufferMemory(base.device, *buffer, *memory, 0);
	void* pointer;
	vkMapMemory(base.device, *memory, 0, memorySize, 0, &pointer);	

	for (size_t dI = 0; dI < ds.size(); dI++)
	{
		ds[dI]->pointer = pointer;

		vkhelper::createDescriptorSet(base.device, ds[dI]->ds, ds[dI]->setCount);

		std::vector<VkWriteDescriptorSet> writes(ds[dI]->setCount, {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET});
		std::vector<VkDescriptorBufferInfo> buffInfos(ds[dI]->setCount);

		for (size_t i = 0; i < ds[dI]->setCount; i++)
		{
			buffInfos[i].buffer = *buffer;
			buffInfos[i].offset = ds[dI]->offset + (ds[dI]->slotSize * i);
			buffInfos[i].range = ds[dI]->slotSize;

			writes[i].dstSet = ds[dI]->ds.sets[i];
			writes[i].pBufferInfo = buffInfos.data() + i;
			writes[i].dstBinding = 0;
			writes[i].dstArrayElement = 0;
			if(ds[dI]->type == DS::BufferType::Storage)
				writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			else if(ds[dI]->type == DS::BufferType::Uniform)
				writes[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writes[i].descriptorCount = 1;
		}
		vkUpdateDescriptorSets(base.device, writes.size(), writes.data(), 0, nullptr);
	}
}

glm::mat4 vkhelper::calcMatFromRect(glm::vec4 rect, float rotate)
{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(rect.x, rect.y, 0.0f));

		if(rotate != 0)
		{
			model = glm::translate(model, glm::vec3(0.5 * rect.z, 0.5 * rect.w, 0.0));
			model = glm::rotate(model, glm::radians(rotate), glm::vec3(0.0, 0.0, 1.0));
			model = glm::translate(model, glm::vec3(-0.5 * rect.z, -0.5 * rect.w, 0.0)); 
		}
		model = glm::scale(model, glm::vec3(rect.z, rect.w, 1.0f)); 
		return model;
	}