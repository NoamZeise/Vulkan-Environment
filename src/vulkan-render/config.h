#ifndef VULKAN_RENDERER_CONFIG_H
#define VULKAN_RENDERER_CONFIG_H


//#define NDEBUG //uncomment for release mode

namespace settings
{

const bool SRGB = false;
const bool MIP_MAPPING = true;
const bool PIXELATED = false;
const bool VSYNC = true;
const bool MULTISAMPLING = true;
const bool SAMPLE_SHADING = true;

#ifndef NDEBUG
const bool ERROR_ONLY = true;
#endif
}


#endif