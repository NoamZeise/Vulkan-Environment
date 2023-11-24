#ifndef OUTFACING_TEXTURE_LOADER
#define OUTFACING_TEXTURE_LOADER

#include "resources.h"

class TextureLoader {
 public:
    virtual Resource::Texture load(std::string path) = 0;
    /// takes ownership of data
    virtual Resource::Texture load(unsigned char* data,
					  int width,
					  int height,
					  int nrChannels) = 0;
};

#endif /* OUTFACING_TEXTURE_LOADER */

