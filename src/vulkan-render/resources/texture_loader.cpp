#include "texture_loader.h"

namespace Resource
{

TextureLoader::TextureLoader(Base base, VkCommandPool pool)
{
	this->base = base;
	this->pool = pool;
}

TextureLoader::~TextureLoader()
{
	UnloadTextures();
}

void TextureLoader::UnloadTextures()
{
	for(auto& tex: texToLoad)
	{
		if (tex.path != "NULL")
			stbi_image_free(tex.pixelData);
		else
			delete[] tex.pixelData;
	}
	if (textures.size() <= 0)
		return;
	for (auto& tex : textures)
	{
		vkDestroyImageView(base.device, tex.view, nullptr);
		vkDestroyImage(base.device, tex.image, nullptr);
	}
	vkDestroySampler(base.device, textureSampler, nullptr);
	vkFreeMemory(base.device, memory, nullptr);

	textures.clear();
}

Texture TextureLoader::LoadTexture(std::string path)
{
	for(unsigned int i = 0; i > texToLoad.size(); i++)
	{
		if(texToLoad[i].path == path)
		{
			return Texture(i, glm::vec2(texToLoad[i].width, texToLoad[i].height), path);
		}
	}

#ifndef NDEBUG
	std::cout << "loading texture: " << path << std::endl;
#endif
	texToLoad.push_back({ std::string(path.c_str()) });
	TempTexture* tex = &texToLoad.back();
	tex->pixelData = stbi_load(tex->path.c_str(), &tex->width, &tex->height, &tex->nrChannels, 4);
	if (!tex->pixelData)
		throw std::runtime_error("failed to load texture at " + path);

	tex->nrChannels = 4;

	tex->fileSize = tex->width * tex->height * tex->nrChannels;

	if(settings::SRGB)
		tex->format = VK_FORMAT_R8G8B8A8_SRGB;
	else
		tex->format = VK_FORMAT_R8G8B8A8_UNORM;

	return Texture((unsigned int)(texToLoad.size() - 1), glm::vec2(tex->width, tex->height), path);
}

	//takes ownership of data
Texture TextureLoader::LoadTexture(unsigned char* data, int width, int height, int nrChannels)
{
	texToLoad.push_back({ "NULL" });
	TempTexture* tex = &texToLoad.back();
	tex->pixelData = data;
	tex->width = width;
	tex->height = height;
	tex->nrChannels = nrChannels;
	tex->fileSize = tex->width * tex->height * tex->nrChannels;

	if(nrChannels != 4)
		throw std::runtime_error("nrChannels for pixel data not 4");
	if(settings::SRGB)
		tex->format = VK_FORMAT_R8G8B8A8_SRGB;
	else
		tex->format = VK_FORMAT_R8G8B8A8_UNORM;

	return Texture((unsigned int)(texToLoad.size() - 1), glm::vec2(tex->width, tex->height), "NULL");
}

void TextureLoader::endLoading()
{
	if (texToLoad.size() <= 0)
		return;

	if (texToLoad.size() > MAX_TEXTURES_SUPPORTED)
		throw std::runtime_error("not enough storage for textures");
	textures.resize(texToLoad.size());

	VkDeviceSize totalFilesize = 0;
	for (const auto& tex : texToLoad)
		totalFilesize += tex.fileSize;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;

	vkhelper::createBufferAndMemory(base, totalFilesize, &stagingBuffer, &stagingMemory,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vkBindBufferMemory(base.device, stagingBuffer, stagingMemory, 0);
	void* pMem;
	vkMapMemory(base.device, stagingMemory, 0, totalFilesize, 0, &pMem);

	//all during loop:

	//move image pixel data to buffer
	VkDeviceSize finalMemSize = 0;
	VkMemoryRequirements memreq;
	VkDeviceSize bufferOffset = 0;

	uint32_t memoryTypeBits = 0;
	uint32_t minMips = UINT32_MAX;
	for (size_t i = 0; i < texToLoad.size(); i++)
	{
		std::memcpy(static_cast<char*>(pMem) + bufferOffset, texToLoad[i].pixelData, texToLoad[i].fileSize);

		if (texToLoad[i].path != "NULL")
			stbi_image_free(texToLoad[i].pixelData);
		else
			delete[] texToLoad[i].pixelData;
		texToLoad[i].pixelData = nullptr;

		bufferOffset += texToLoad[i].fileSize;

		textures[i] = LoadedTexture(texToLoad[i]);
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(base.physicalDevice, texToLoad[i].format, &formatProperties);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
			|| !(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)
			|| !settings::MIP_MAPPING)
			textures[i].mipLevels = 1;
		//get smallest mip levels of any texture
		if (textures[i].mipLevels < minMips)
			minMips = textures[i].mipLevels;


		VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = textures[i].width;
		imageInfo.extent.height = textures[i].height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = textures[i].mipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.format = texToLoad[i].format;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT; //todo add multsampling

		if (vkCreateImage(base.device, &imageInfo, nullptr, &textures[i].image) != VK_SUCCESS)
			throw std::runtime_error("failed to create image from texture at: " + texToLoad[i].path);


		vkGetImageMemoryRequirements(base.device, textures[i].image, &memreq);
		memoryTypeBits |= memreq.memoryTypeBits;
		textures[i].imageMemSize = memreq.size;

		if (textures[i].imageMemSize %  memreq.alignment != 0)
			textures[i].imageMemSize = textures[i].imageMemSize + memreq.alignment
			- (textures[i].imageMemSize % memreq.alignment);

		finalMemSize += textures[i].imageMemSize;
	}
	//create device local memory for permanent storage of images
	vkhelper::createMemory(base.device, base.physicalDevice, finalMemSize, &memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryTypeBits);

	//transition image to required format
	VkCommandBufferAllocateInfo cmdAllocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdAllocInfo.commandBufferCount = 1;
	cmdAllocInfo.commandPool = pool;
	VkCommandBuffer tempCmdBuffer;
	vkAllocateCommandBuffers(base.device, &cmdAllocInfo, &tempCmdBuffer);

	VkCommandBufferBeginInfo cmdBeginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(tempCmdBuffer, &cmdBeginInfo);

	VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; //for mipmapping
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	VkBufferImageCopy region{};
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	bufferOffset = 0;
	VkDeviceSize finalMemoryOffset = 0;
	for (int i = 0; i < textures.size(); i++)
	{
		vkBindImageMemory(base.device, textures[i].image, memory, finalMemoryOffset);
		finalMemoryOffset += textures[i].imageMemSize;

		barrier.image = textures[i].image;
		barrier.subresourceRange.levelCount = textures[i].mipLevels;
		vkCmdPipelineBarrier(tempCmdBuffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr, 0, nullptr, 1, &barrier);

		region.imageExtent = { textures[i].width, textures[i].height, 1 };
		region.bufferOffset = bufferOffset;
		bufferOffset += texToLoad[i].fileSize;

		vkCmdCopyBufferToImage(tempCmdBuffer, stagingBuffer, textures[i].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &region);
	}

	if (vkEndCommandBuffer(tempCmdBuffer) != VK_SUCCESS)
		throw std::runtime_error("failed to end command buffer");
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &tempCmdBuffer;
	vkQueueSubmit(base.queue.graphicsPresentQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(base.queue.graphicsPresentQueue);

	vkUnmapMemory(base.device, stagingMemory);
	vkDestroyBuffer(base.device, stagingBuffer, nullptr);
	vkFreeMemory(base.device, stagingMemory, nullptr);

	vkResetCommandPool(base.device, pool, 0);
	vkBeginCommandBuffer(tempCmdBuffer, &cmdBeginInfo);

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	for (const auto& tex : textures)
	{
		barrier.image = tex.image;
		int32_t mipW = tex.width;
		int32_t mipH = tex.height;

		for (uint32_t i = 1; i < tex.mipLevels; i++) //start at one as 0 is original image
		{
			//transfer previous image to be optimal for image transfer source
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(tempCmdBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr, 0, nullptr, 1, &barrier);

			//blit current image from previous one
			VkImageBlit blit{};
			//src info
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipW, mipH, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			//dst info
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipW > 1 ? mipW / 2 : 1, mipH > 1 ? mipH / 2: 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(tempCmdBuffer, tex.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				tex.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

			//change previous image layout to shader read only
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(tempCmdBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr, 0, nullptr, 1, &barrier);

			if (mipW > 1) mipW /= 2;
			if (mipH > 1) mipH /= 2;
		}
		//transition last mip level to shader read only
		barrier.subresourceRange.baseMipLevel = tex.mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(tempCmdBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr, 0, nullptr, 1, &barrier);
	}

	if (vkEndCommandBuffer(tempCmdBuffer) != VK_SUCCESS)
		throw std::runtime_error("failed to end command buffer");
	vkQueueSubmit(base.queue.graphicsPresentQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(base.queue.graphicsPresentQueue);

	//create image views
	for (size_t i = 0; i < textures.size(); i++)
	{
		VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		viewInfo.image = textures[i].image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = texToLoad[i].format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = textures[i].mipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(base.device, &viewInfo, nullptr, &textures[i].view) != VK_SUCCESS)
			throw std::runtime_error("Failed to create image view from texture at: " + texToLoad[i].path);
	}
    
    textureSampler = vkhelper::createTextureSampler(base.device, base.physicalDevice, static_cast<float>(minMips), base.features.samplerAnisotropy);

	vkFreeCommandBuffers(base.device, pool, 1, &tempCmdBuffer);

	for(uint32_t i = 0; i < MAX_TEXTURES_SUPPORTED; i++)
	{
		imageViews[i] = _getImageView(i);
	}

	texToLoad.clear();
}

VkImageView TextureLoader::_getImageView(uint32_t texID)
{
	if (texID < textures.size())
		return textures[texID].view;
	else if (textures.size() > 0)
		return textures[0].view;
	else
		throw std::runtime_error("no textures to replace error id with");
}

}//end namesapce
