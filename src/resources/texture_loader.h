#ifndef TEXTURE_LOADER_H
#define TEXTURE_LOADER_H

#include <vector>
#include <string>

#include "../render_structs/device_state.h"
#include <graphics/render_config.h>
#include <graphics/resources.h>

namespace Resource {

  class TextureLoader {
  public:
      TextureLoader(DeviceState base, VkCommandPool pool, RenderConfig config);
      ~TextureLoader();
      void UnloadTextures();
      Texture LoadTexture(std::string path);
      // takes ownership of data
      Texture LoadTexture(unsigned char* data, int width, int height, int nrChannels);
      void endLoading();
      VkSampler* getSamplerP() { return &textureSampler; }
      VkImageView* getImageViewsP() { return &imageViews[0]; }
      
  private:
      struct TempTexture {
	  std::string path;
	  unsigned char* pixelData;
	  int width;
	  int height;
	  int nrChannels;
	  VkFormat format;
	  VkDeviceSize fileSize;
      };
      
    struct LoadedTexture {
	LoadedTexture(){}
	LoadedTexture(TempTexture tex) {
	    width = tex.width;
	    height = tex.height;
	    mipLevels = (int)std::floor(std::log2(width > height ? width : height)) + 1;
	}
	uint32_t width;
	uint32_t height;
	VkImage image;
	VkImageView view;
	uint32_t mipLevels;
	VkDeviceSize imageMemSize;
	VkDeviceSize imageMemOffset;
    };

      VkDeviceSize stageTexDataCreateImages(VkBuffer &stagingBuffer,
					     VkDeviceMemory &stagingMemory,
					     uint32_t *pFinalMemType,
					     uint32_t *pMinimumMipmapLevel);
      void copyTextureToStagingBuffer(void* pMem, VkDeviceSize *pOffset,
					      TempTexture &toLoad);
      void createImageForTexture(VkDevice device, LoadedTexture &tex, VkFormat imgFormat,
				 VkMemoryRequirements *pMemreq);
      void textureDataStagingToFinal(VkBuffer stagingBuffer,
						VkCommandBuffer &cmdbuff);
      void createTexMipMaps(VkCommandBuffer &cmdBuff, const LoadedTexture &tex);
      
      VkImageView _getImageView(uint32_t texID);
      
      bool srgb;
      bool mipmapping;
      bool useNearestTextureFilter;
      
      DeviceState base;
      VkCommandPool pool;
      
      std::vector<TempTexture> texToLoad;
      std::vector<LoadedTexture> textures;
      VkDeviceMemory memory;
      VkImageView imageViews[MAX_TEXTURES_SUPPORTED];
      VkSampler textureSampler;
  };
}
#endif
