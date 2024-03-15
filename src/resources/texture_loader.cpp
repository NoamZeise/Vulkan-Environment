#include "texture_loader.h"

#include <cstring>
#include "../logger.h"
#include "../vkhelper.h"
#include "../parts/images.h"
#include "../parts/command.h"
#include "../parts/threading.h"

const VkFilter MIPMAP_FILTER = VK_FILTER_LINEAR;

struct TextureInGPU {
    TextureInGPU(VkDevice device, StagedTex tex, bool srgb) {
	this->device = device;
	width = tex.width;
	height = tex.height;
	mipLevels = (int)std::floor(std::log2(width > height ? width : height)) + 1;
	if(tex.nrChannels != 4)
	    throw std::runtime_error("GPU Tex has unsupport no. of channels!");
	if(srgb)
	    format = VK_FORMAT_R8G8B8A8_SRGB;
	else
	    format = VK_FORMAT_R8G8B8A8_UNORM;
    }
    ~TextureInGPU() {
	vkDestroyImageView(device, view, nullptr);
	vkDestroyImage(device, image, nullptr);	  
    }
    VkDevice device;
    uint32_t imageViewIndex = 0;
    uint32_t width;
    uint32_t height;
    VkImage image;
    VkImageView view;
    uint32_t mipLevels;
    VkFormat format;
    VkDeviceSize imageMemSize;
    VkDeviceSize imageMemOffset;
    VkResult createImage(VkDevice device, VkMemoryRequirements *pMemreq);
    void createMipMaps(VkCommandBuffer &cmdBuff);
    VkResult createImageView(VkDevice device);
};
  
TexLoaderVk::TexLoaderVk(DeviceState base, VkCommandPool cmdpool,
			 Resource::Pool pool, RenderConfig config)
    : InternalTexLoader(pool, config) {
    this->base = base;
    this->cmdpool = cmdpool;
    checkResultAndThrow(part::create::Fence(base.device, &loadedFence, false),
			"failed to create finish load semaphore in texLoader");
}

TexLoaderVk::~TexLoaderVk() {
    vkDestroyFence(base.device, loadedFence, nullptr);
    clearGPU();
}

void TexLoaderVk::clearGPU() {
    if (textures.size() <= 0)
	return;
    for (auto& tex : textures)
	delete tex;
    vkFreeMemory(base.device, memory, nullptr);
    textures.clear();
}

float TexLoaderVk::getMinMipmapLevel() {
    return (float)minimumMipmapLevel;
}

void TexLoaderVk::loadGPU() {
    if(staged.size() <= 0)
	return;
    
    clearGPU();
    textures.resize(staged.size());
    LOG("end texture load, loading " << staged.size() << " textures to GPU");

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    uint32_t memoryTypeBits;
    VkDeviceSize finalMemSize = stageTexDataCreateImages(
	    stagingBuffer, stagingMemory, &memoryTypeBits);

    LOG("creating final memory buffer " << finalMemSize << " bytes");
    
    // create final memory
    checkResultAndThrow(vkhelper::allocateMemory(base.device, base.physicalDevice,
						 finalMemSize, &memory,
						 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
						 memoryTypeBits),
			"Failed to allocate memeory for final texture storage");

    LOG("creating temp cmdbuff");

    VkCommandBuffer tempCmdBuffer;
    checkResultAndThrow(part::create::CommandBuffer(base.device, cmdpool, &tempCmdBuffer),
			"failed to create temporary command buffer in end texture loading");
    VkCommandBufferBeginInfo cmdBeginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    checkResultAndThrow(vkBeginCommandBuffer(tempCmdBuffer, &cmdBeginInfo),
			"failed to begin staging texture data command buffer");

    // move texture data from staging memory to final memory
    textureDataStagingToFinal(stagingBuffer, tempCmdBuffer);

    checkResultAndThrow(vkEndCommandBuffer(tempCmdBuffer),
			"failed to end cmdbuff for moving tex data");

    LOG("submitting cmdBuffer");
    
    checkResultAndThrow(vkhelper::submitCmdBuffAndWait(
				base.device,
				base.queue.graphicsPresentQueue, &tempCmdBuffer, loadedFence,
				&graphicsPresentMutex),
			"failed to move tex datat to gpu");
    
    LOG("finished moving textures to final memory location");

    checkResultAndThrow(vkResetCommandPool(base.device, cmdpool, 0),
			"Failed to reset command pool in end tex loading");

    // free staging buffer/memory 
    vkUnmapMemory(base.device, stagingMemory);
    vkDestroyBuffer(base.device, stagingBuffer, nullptr);
    vkFreeMemory(base.device, stagingMemory, nullptr);
    
    checkResultAndThrow(vkBeginCommandBuffer(tempCmdBuffer, &cmdBeginInfo),
			"Failed to begin mipmap creation command buffer");

    for (auto& tex : textures)
	tex->createMipMaps(tempCmdBuffer);

    checkResultAndThrow(vkEndCommandBuffer(tempCmdBuffer),
			"failed to end command buffer");

    checkResultAndThrow(
	    vkhelper::submitCmdBuffAndWait(base.device, base.queue.graphicsPresentQueue,
					   &tempCmdBuffer, loadedFence,
					   &graphicsPresentMutex),
			"failed to sumbit mipmap creation commands");
    
    LOG("finished creating mip maps");
    
    //create image views
    for (auto &tex: textures)
	checkResultAndThrow(
		tex->createImageView(base.device), 
		"Failed to create image view from texture");

    LOG("finished creating image views and texture samplers");
    vkFreeCommandBuffers(base.device, cmdpool, 1, &tempCmdBuffer);
    staged.clear();
    LOG("finished loading textures");
}

uint32_t TexLoaderVk::getImageCount() { return textures.size(); }

VkImageView TexLoaderVk::getImageViewSetIndex(uint32_t texID, uint32_t imageViewIndex) {
    if (texID < textures.size()) {
	textures[texID]->imageViewIndex = imageViewIndex;
	return textures[texID]->view;
    }
    else if (textures.size() > 0) {
	LOG_ERROR("Requested texture ID was out of range");
	return textures[0]->view;
    }
    else
	throw std::runtime_error("no textures to replace error id with");
}

unsigned int TexLoaderVk::getViewIndex(Resource::Texture tex) {
    if(tex.pool != this->pool) {
	LOG_ERROR("tex loader - getViewIndex: Texture does not belong to this resource pool");
	return Resource::NULL_ID;
    }
    if(tex.ID == Resource::NULL_ID)
	return tex.ID;
    if (tex.ID < textures.size())
	return textures[tex.ID]->imageViewIndex;
      
    LOG_ERROR("View Index's texture was out of range. given id: " <<
	      tex.ID << " , size: " << textures.size() << " . Returning 0.");
    return 0;
}

/// ---- GPU loading helpers ---

void addImagePipelineBarrier(VkCommandBuffer &cmdBuff,
			     VkImageMemoryBarrier &barrier,
			     VkPipelineStageFlags srcStageMask,
			     VkPipelineStageFlags dstStageMask) {
    vkCmdPipelineBarrier(cmdBuff,
			 srcStageMask, dstStageMask,
			 0, 0, nullptr, 0, nullptr,
			 1, &barrier);
}

bool formatSupportsMipmapping(VkPhysicalDevice physicalDevice, VkFormat format) {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format,
					&formatProperties);
    return (formatProperties.optimalTilingFeatures &
	    VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
	&& (formatProperties.optimalTilingFeatures &
	    VK_FORMAT_FEATURE_BLIT_DST_BIT);
}

VkResult TextureInGPU::createImage(VkDevice device, VkMemoryRequirements *pMemreq) {
    return part::create::Image(device, &this->image, pMemreq,
			       VK_IMAGE_USAGE_SAMPLED_BIT |
			       VK_IMAGE_USAGE_TRANSFER_DST_BIT |
			       VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			       VkExtent2D { this->width, this->height },
			       this->format,
			       VK_SAMPLE_COUNT_1_BIT, this->mipLevels);
}

VkDeviceSize TexLoaderVk::stageTexDataCreateImages(VkBuffer &stagingBuffer,
						   VkDeviceMemory &stagingMemory,
						   uint32_t *pFinalMemType) {
    VkDeviceSize totalDataSize = 0;
    for(const auto& tex: staged)
	totalDataSize += tex.filesize;

    LOG("creating staging buffer for textures. size: " << totalDataSize << " bytes");
    checkResultAndThrow(vkhelper::createBufferAndMemory(
				base, totalDataSize, &stagingBuffer, &stagingMemory,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
			"Failed to create staging memory for texture data");
      
    vkBindBufferMemory(base.device, stagingBuffer, stagingMemory, 0);
    void* pMem;
    vkMapMemory(base.device, stagingMemory, 0, totalDataSize, 0, &pMem);

    VkDeviceSize finalMemSize = 0;
    VkMemoryRequirements memreq;
    VkDeviceSize bufferOffset = 0;

    *pFinalMemType = 0;
    minimumMipmapLevel = UINT32_MAX;
    for (size_t i = 0; i < staged.size(); i++) {
	std::memcpy(static_cast<char*>(pMem) + bufferOffset,
		    staged[i].data,
		    staged[i].filesize);
	staged[i].deleteData();
	bufferOffset += staged[i].filesize;
	
	textures[i] = new TextureInGPU(base.device, staged[i], srgb);
	if (!mipmapping ||
	    !formatSupportsMipmapping(base.physicalDevice, textures[i]->format))
	    textures[i]->mipLevels = 1;
	  
	checkResultAndThrow(textures[i]->createImage(base.device, &memreq),
			    "failed to create image in texture loader"
			    "for texture at index " + std::to_string(i));

	//get smallest mip levels of any texture
	if (textures[i]->mipLevels < minimumMipmapLevel)
	    minimumMipmapLevel = textures[i]->mipLevels;
	  
	*pFinalMemType |= memreq.memoryTypeBits;
	finalMemSize = vkhelper::correctMemoryAlignment(finalMemSize, memreq.alignment);
	textures[i]->imageMemOffset = finalMemSize;
	textures[i]->imageMemSize = vkhelper::correctMemoryAlignment(
		memreq.size, memreq.alignment);
	finalMemSize += textures[i]->imageMemSize;
    }
    LOG("successfully copied textures to staging buffer");
    return finalMemSize;
}

VkImageMemoryBarrier initialBarrierSettings();

// transition image to mipmapping format + copy data from staging buffer to gpu memory
// and bind to texture image.
void TexLoaderVk::textureDataStagingToFinal(VkBuffer stagingBuffer,
					    VkCommandBuffer &cmdbuff) {
    VkImageMemoryBarrier barrier = initialBarrierSettings();
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; // this layout used for mipmapping
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
      
    VkDeviceSize bufferOffset = 0;
    for (int i = 0; i < textures.size(); i++) {
	vkBindImageMemory(base.device, textures[i]->image, memory, textures[i]->imageMemOffset);
	barrier.image = textures[i]->image;
	barrier.subresourceRange.levelCount = textures[i]->mipLevels;
	addImagePipelineBarrier(cmdbuff, barrier,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT);
	region.imageExtent = { textures[i]->width, textures[i]->height, 1 };
	region.bufferOffset = bufferOffset;
	bufferOffset += staged[i].filesize;

	vkCmdCopyBufferToImage(cmdbuff, stagingBuffer, textures[i]->image,
			       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			       1, &region);
    }
}

/// --- Mipmapping ---
  
void dstToSrcBarrier(VkImageMemoryBarrier *barrier);
  
void srcToReadOnlyBarrier(VkImageMemoryBarrier *barrier);
  
VkImageBlit getMipmapBlit(int32_t currentW, int32_t currentH, int destMipLevel);

void TextureInGPU::createMipMaps(VkCommandBuffer &cmdBuff) {
    VkImageMemoryBarrier barrier = initialBarrierSettings();
    barrier.image = this->image;
    int mipW = this->width;
    int mipH = this->height;
    for(int i = 1; i < this->mipLevels; i++) { //start at 1 (0 would be full size)
	// change image layout to be ready for mipmapping
	barrier.subresourceRange.baseMipLevel = i - 1;
	dstToSrcBarrier(&barrier);
	addImagePipelineBarrier(cmdBuff, barrier,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT);

	VkImageBlit blit = getMipmapBlit(mipW, mipH, i);
	vkCmdBlitImage(cmdBuff, this->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		       this->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
		       MIPMAP_FILTER);
	  
	srcToReadOnlyBarrier(&barrier);
	addImagePipelineBarrier(cmdBuff, barrier,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	if(mipW > 1) mipW /= 2;
	if(mipH > 1) mipH /= 2;
    }
    //transition last mipmap lvl from dst to shader read only
    srcToReadOnlyBarrier(&barrier);
    barrier.subresourceRange.baseMipLevel = this->mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    addImagePipelineBarrier(cmdBuff, barrier,
			    VK_PIPELINE_STAGE_TRANSFER_BIT,
			    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

VkImageBlit getMipmapBlit(int32_t currentW, int32_t currentH, int destMipLevel) {
    VkImageBlit blit{};
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1] = { currentW, currentH, 1 };
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = destMipLevel - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = { 0, 0, 0 };
    blit.dstOffsets[1] = {
	currentW > 1 ? currentW / 2 : 1,
	currentH > 1 ? currentH / 2 : 1,
	1 };
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = destMipLevel;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;
    return blit;
}


/// --- image view creation ---

VkResult TextureInGPU::createImageView(VkDevice device) {
    VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.image = this->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = this->format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = this->mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
	
    return vkCreateImageView(device, &viewInfo, nullptr, &this->view);
}

/// --- memory barriers ---
    
VkImageMemoryBarrier initialBarrierSettings() {
    VkImageMemoryBarrier barrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;
    return barrier;
}

void dstToSrcBarrier(VkImageMemoryBarrier *barrier) {
      barrier->oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier->newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier->srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier->dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  }

void srcToReadOnlyBarrier(VkImageMemoryBarrier *barrier) {
    barrier->oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier->newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier->srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier->dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
}
