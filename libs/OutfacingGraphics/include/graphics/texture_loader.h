#ifndef OUTFACING_TEXTURE_LOADER
#define OUTFACING_TEXTURE_LOADER

#include <graphics/resources.h>

class TextureLoader {
 public:
    virtual Resource::Texture LoadTexture(std::string path) = 0;
    /// takes ownership of data
    virtual Resource::Texture LoadTexture(unsigned char* data,
					  int width,
					  int height,
					  int nrChannels) = 0;
};

#endif /* OUTFACING_TEXTURE_LOADER */

