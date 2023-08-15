#include "texture_loader.h"

#include <stdexcept>
#include <cmath>
#include <cstring>
#include <iostream>

#include <resource_loader/stb_image.h>
#include "../vkhelper.h"
#include "../parts/images.h"
#include "../parts/command.h"

#include "../logger.h"

const VkFilter MIPMAP_FILTER = VK_FILTER_LINEAR;

namespace Resource {

  /// --- interal texture storage ---
  
  struct TextureInMemory {
      std::string path;
      unsigned char* pixelData;
      int width;
      int height;
      int nrChannels;
      VkFormat format;
      VkDeviceSize fileSize;
      
      void copyToStagingMemAndFreePixelData(void* pMem, VkDeviceSize *pOffset);
  };

  struct TextureInGPU {
      TextureInGPU(){}
      TextureInGPU(TextureInMemory tex) {
	  width = tex.width;
	  height = tex.height;
	  mipLevels = (int)std::floor(std::log2(width > height ? width : height)) + 1;
	  format = tex.format;
      }
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

  /// --- texture loader ---
  
  TextureLoader::TextureLoader(DeviceState base, VkCommandPool pool, RenderConfig config) {
    this->srgb = config.srgb;
    this->mipmapping = config.mip_mapping;
    this->useNearestTextureFilter = config.texture_filter_nearest;
    this->base = base;
    this->pool = pool;
  }

  TextureLoader::~TextureLoader() {
    UnloadTextures();
  }

  void TextureLoader::UnloadTextures() {
      for(auto& tex: texToLoad) {
	  if (tex.path != "NULL")
	      stbi_image_free(tex.pixelData);
	  else
	      delete[] tex.pixelData;
      }
      if (textures.size() <= 0)
	  return;
      for (auto& tex : textures) {
	  vkDestroyImageView(base.device, tex.view, nullptr);
	  vkDestroyImage(base.device, tex.image, nullptr);
      }
      vkDestroySampler(base.device, textureSampler, nullptr);
      vkFreeMemory(base.device, memory, nullptr);
      textures.clear();
      texToLoad.clear();
  }
  
  Texture TextureLoader::LoadTexture(std::string path) {
      for(unsigned int i = 0; i > texToLoad.size(); i++) {
	  if(texToLoad[i].path == path) {
	      return Texture(i, glm::vec2(texToLoad[i].width, texToLoad[i].height), path);
	  }
      }
      
      LOG("loading texture: " << path);
      
      texToLoad.push_back({ std::string(path.c_str()) });
      TextureInMemory* tex = &texToLoad.back();
      tex->pixelData = stbi_load(tex->path.c_str(), &tex->width, &tex->height, &tex->nrChannels, 4);
      if (!tex->pixelData) {
	  LOG_ERROR("failed to load texture - path: " << path);
	  throw std::runtime_error("failed to load texture at " + path);
      }
      
      tex->nrChannels = 4;
      
      tex->fileSize = tex->width * tex->height * tex->nrChannels;
      
      if(srgb)
	  tex->format = VK_FORMAT_R8G8B8A8_SRGB;
      else
	  tex->format = VK_FORMAT_R8G8B8A8_UNORM;
      
      LOG("  --- successfully loaded at ID: " << (int)(texToLoad.size() - 1));
      
      return Texture((unsigned int)(texToLoad.size() - 1),
		     glm::vec2(tex->width, tex->height), path);
  }

  Texture TextureLoader::LoadTexture(unsigned char* data, int width, int height, int nrChannels) {
    texToLoad.push_back({ "NULL" });
    TextureInMemory* tex = &texToLoad.back();
    tex->pixelData = data;
    tex->width = width;
    tex->height = height;
    tex->nrChannels = nrChannels;
    tex->fileSize = tex->width * tex->height * tex->nrChannels;

    if(nrChannels != 4)
      throw std::runtime_error("nrChannels for pixel data not 4");
    if(srgb)
      tex->format = VK_FORMAT_R8G8B8A8_SRGB;
    else
      tex->format = VK_FORMAT_R8G8B8A8_UNORM;

    return Texture((unsigned int)(texToLoad.size() - 1), glm::vec2(tex->width, tex->height), "NULL");
  }

  
  /// --- loading textures to GPU ---

  void TextureLoader::endLoading() {
    if (texToLoad.size() <= 0)
	return;
    
    textures.resize(texToLoad.size());

    LOG("end texture load, loading " << texToLoad.size() << " textures to GPU");

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    uint32_t memoryTypeBits;
    uint32_t minimumMipmapLevel;
    VkDeviceSize finalMemSize = stageTexDataCreateImages(
	    stagingBuffer, stagingMemory, &memoryTypeBits, &minimumMipmapLevel);

    LOG("creating final memory buffer " << finalMemSize << " bytes");
    
    // create final memory
    checkResultAndThrow(vkhelper::allocateMemory(base.device, base.physicalDevice,
						 finalMemSize, &memory,
						 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
						 memoryTypeBits),
			"Failed to allocate memeory for final texture storage");

    VkCommandBuffer tempCmdBuffer;
    checkResultAndThrow(part::create::CommandBuffer(base.device, pool, &tempCmdBuffer),
			"failed to create temporary command buffer in end texture loading");
    VkCommandBufferBeginInfo cmdBeginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    checkResultAndThrow(vkBeginCommandBuffer(tempCmdBuffer, &cmdBeginInfo),
			"failed to begin staging texture data command buffer");

    // move texture data from staging memory to final memory
    textureDataStagingToFinal(stagingBuffer, tempCmdBuffer);

    vkhelper::endCmdBufferSubmitAndWait(tempCmdBuffer,
					base.queue.graphicsPresentQueue);
    
    LOG("finished moving textures to final memory location");

    checkResultAndThrow(vkResetCommandPool(base.device, pool, 0),
			"Failed to reset command pool in end tex loading");

    // free staging buffer/memory 
    vkUnmapMemory(base.device, stagingMemory);
    vkDestroyBuffer(base.device, stagingBuffer, nullptr);
    vkFreeMemory(base.device, stagingMemory, nullptr);
    
    checkResultAndThrow(vkBeginCommandBuffer(tempCmdBuffer, &cmdBeginInfo),
			"Failed to begin mipmap creation command buffer");

    for (auto& tex : textures)
	tex.createMipMaps(tempCmdBuffer);

    vkhelper::endCmdBufferSubmitAndWait(tempCmdBuffer, base.queue.graphicsPresentQueue);
    
    LOG("finished creating mip maps");
    
    //create image views
    for (auto &tex: textures)
	checkResultAndThrow(
		tex.createImageView(base.device), 
		"Failed to create image view from texture");

    this->textureSampler = vkhelper::createTextureSampler(
	    base.device,
	    base.physicalDevice,
	    static_cast<float>(minimumMipmapLevel),
	    base.features.samplerAnisotropy,
	    useNearestTextureFilter,
	    VK_SAMPLER_ADDRESS_MODE_REPEAT);

    LOG("finished creating image views and texture samplers");

    vkFreeCommandBuffers(base.device, pool, 1, &tempCmdBuffer);

    for(uint32_t i = 0; i < MAX_TEXTURES_SUPPORTED; i++)
	imageViews[i] = _getImageView(i);

    texToLoad.clear();

    LOG("finished loading textures");
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

  void addImagePipelineBarrier(VkCommandBuffer &cmdBuff,
			       VkImageMemoryBarrier &barrier,
			       VkPipelineStageFlags srcStageMask,
			       VkPipelineStageFlags dstStageMask) {
      vkCmdPipelineBarrier(cmdBuff,
			   srcStageMask, dstStageMask,
			   0, 0, nullptr, 0, nullptr,
			   1, &barrier);
  }


  /// --- Texture Data Staging ---

  void TextureInMemory::copyToStagingMemAndFreePixelData(void* pMem, VkDeviceSize *pOffset) {
      std::memcpy(static_cast<char*>(pMem) + *pOffset,
		  this->pixelData,
		  this->fileSize);
	
      if (this->path != "NULL")
	  stbi_image_free(this->pixelData);
      else
	  delete[] this->pixelData;
      this->pixelData = nullptr;
      
      *pOffset += this->fileSize;
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

  VkDeviceSize TextureLoader::stageTexDataCreateImages(VkBuffer &stagingBuffer,
							VkDeviceMemory &stagingMemory,
						        uint32_t *pFinalMemType,
							uint32_t *pMinimumMipmapLevel) {
      VkDeviceSize totalDataSize = 0;
      for(const auto& tex: texToLoad)
	  totalDataSize += tex.fileSize;

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
      *pMinimumMipmapLevel = UINT32_MAX;
      for (size_t i = 0; i < texToLoad.size(); i++) {
	  texToLoad[i].copyToStagingMemAndFreePixelData(pMem, &bufferOffset);
	  
	  textures[i] = TextureInGPU(texToLoad[i]);
	  if (!mipmapping ||
	      !formatSupportsMipmapping(base.physicalDevice, this->texToLoad[i].format))
	      textures[i].mipLevels = 1;
	  
	  checkResultAndThrow(textures[i].createImage(base.device, &memreq),
			      "failed to create image in texture loader"
			      "for texture at index " + std::to_string(i));

	  //get smallest mip levels of any texture
	  if (textures[i].mipLevels < *pMinimumMipmapLevel)
	      *pMinimumMipmapLevel = textures[i].mipLevels;
	  
	  *pFinalMemType |= memreq.memoryTypeBits;
	  finalMemSize = vkhelper::correctMemoryAlignment(finalMemSize, memreq.alignment);
	  textures[i].imageMemOffset = finalMemSize;
	  textures[i].imageMemSize = vkhelper::correctMemoryAlignment(
		  memreq.size, memreq.alignment);
	  finalMemSize += textures[i].imageMemSize;
      }
      LOG("successfully copied textures to staging buffer");
      return finalMemSize;
  }


  /// --- Texture Data Staging to Final

  VkImageMemoryBarrier initialBarrierSettings();

  // transition image to mipmapping format + copy data from staging buffer to gpu memory
  // and bind to texture image.
  void TextureLoader::textureDataStagingToFinal(VkBuffer stagingBuffer,
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
	  vkBindImageMemory(base.device, textures[i].image, memory, textures[i].imageMemOffset);
	  barrier.image = textures[i].image;
	  barrier.subresourceRange.levelCount = textures[i].mipLevels;
	  addImagePipelineBarrier(cmdbuff, barrier,
				  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				  VK_PIPELINE_STAGE_TRANSFER_BIT);
	  region.imageExtent = { textures[i].width, textures[i].height, 1 };
	  region.bufferOffset = bufferOffset;
	  bufferOffset += texToLoad[i].fileSize;

	  vkCmdCopyBufferToImage(cmdbuff, stagingBuffer, textures[i].image,
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

}//end namesapce
