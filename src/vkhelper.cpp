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

glm::mat4 vkhelper::getModel(glm::vec4 drawRect, float rotate)
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

	VkMemoryAllocateInfo memInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memInfo.allocationSize = memReq.size;
	memInfo.memoryTypeIndex = findMemoryIndex(base.physicalDevice, memReq.memoryTypeBits, properties);

	if (vkAllocateMemory(base.device, &memInfo, nullptr, memory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate memory of size " + memReq.size);
}

void vkhelper::createMemory(Base base, VkDeviceSize size, VkDeviceMemory* memory,
	VkMemoryPropertyFlags properties, uint32_t memoryTypeBits)
{
	VkMemoryAllocateInfo memInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memInfo.allocationSize = size;
	memInfo.memoryTypeIndex = findMemoryIndex(base.physicalDevice, memoryTypeBits, properties);

	if (vkAllocateMemory(base.device, &memInfo, nullptr, memory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate memory of size " + size);
}