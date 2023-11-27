#ifndef RESOURCE_TEXTURE_LOADER_H
#define RESOURCE_TEXTURE_LOADER_H

#include <graphics/texture_loader.h>
#include <graphics/render_config.h>
#include <vector>

struct StagedTex {
    unsigned char* data;
    int width, height, nrChannels, filesize;
    std::string path;
    bool pathedTex;
    void deleteData();
};

class InternalTexLoader : public TextureLoader {
public:
    InternalTexLoader(Resource::Pool pool, RenderConfig conf);
    ~InternalTexLoader();
    Resource::Texture load(std::string path) override;
    Resource::Texture load(unsigned char* data,
				  int width,
				  int height,
				  int nrChannels) override;

    virtual void loadGPU() = 0;
    void clearStaged();
    virtual void clearGPU() = 0;

    virtual unsigned int getViewIndex(Resource::Texture tex) { return tex.ID; }

 protected:
    bool srgb, mipmapping, filterNearest;
    Resource::Pool pool;
    int desiredChannels = 4;

    std::vector<StagedTex> staged;
};


#endif
