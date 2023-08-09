#ifndef TEXTURE_LOADER_H
#define TEXTURE_LOADER_H

#include <vector>
#include <string>

#include "../render_structs/device_state.h"
#include <graphics/render_config.h>
#include <graphics/resources.h>

namespace Resource {

  //for internal texture storage
  struct TempTexture;
  struct LoadedTexture;

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
      
      VkDeviceSize stageTexDataCreateImages(VkBuffer &stagingBuffer,
					    VkDeviceMemory &stagingMemory,
					    uint32_t *pFinalMemType,
					    uint32_t *pMinimumMipmapLevel);
      void textureDataStagingToFinal(VkBuffer stagingBuffer,
				     VkCommandBuffer &cmdbuff);
      
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
