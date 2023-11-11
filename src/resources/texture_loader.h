#ifndef TEXTURE_LOADER_H
#define TEXTURE_LOADER_H

#include <vector>
#include <stdexcept>

#include "../device_state.h"
#include <resource_loader/texture_loader.h>

struct TextureInGPU;

class TexLoaderVk : public InternalTexLoader {
public:
    TexLoaderVk(DeviceState base, VkCommandPool pool,
		Resource::ResourcePool resPool, RenderConfig config);
    void clearGPU() override;
    void loadGPU() override;
    float getMinMipmapLevel();
    uint32_t getImageCount();
    VkImageView getImageViewSetIndex(uint32_t texID, uint32_t imageViewIndex);
    uint32_t getViewIndex(uint32_t texID);
      
private:
    VkDeviceSize stageTexDataCreateImages(VkBuffer &stagingBuffer,
					  VkDeviceMemory &stagingMemory,
					  uint32_t *pFinalMemType);
    void textureDataStagingToFinal(VkBuffer stagingBuffer,
				   VkCommandBuffer &cmdbuff);
            
    DeviceState base;
    VkCommandPool cmdpool;
    std::vector<TextureInGPU*> textures;
    VkDeviceMemory memory;
    uint32_t minimumMipmapLevel;
};

#endif
