#ifndef RESOURCE_TEXTURE_LOADER_H
#define RESOURCE_TEXTURE_LOADER_H

#include <graphics/resources.h>
#include <graphics/render_config.h>
#include <vector>

template<typename StgTex, typename GpuTex>
class TextureLoader {
    TextureLoader(Resource::ResourcePool pool, RenderConfig conf);
    Resource::Texture LoadTexture(std::string path);
    Resource::Texture LoadTexture(unsigned char* data, int width, int height, int nrChannels);

    void loadToGpu();
    void clearStaged();
    void clearGPU();

 private:
    bool srgb, mipmapping, filterNearest;
    Resource::ResourcePool pool;
    std::vector<StgTex> staged;
    std::vector<GpuTex> gpu;
};


#endif
