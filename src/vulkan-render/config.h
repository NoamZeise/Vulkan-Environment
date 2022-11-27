#ifndef VULKAN_RENDERER_CONFIG_H
#define VULKAN_RENDERER_CONFIG_H


//#define NDEBUG
//#define NO_ASSIMP

namespace settings
{
const bool SRGB = false;
 
const bool MIP_MAPPING = true;
const bool PIXELATED = false;
const bool MULTISAMPLING = true;
const bool SAMPLE_SHADING = true;

#ifndef NDEBUG
const bool ERROR_ONLY = false;
#endif
}


#endif
