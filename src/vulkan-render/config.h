#ifndef VULKAN_RENDERER_CONFIG_H
#define VULKAN_RENDERER_CONFIG_H


//#define NDEBUG
//#define NO_ASSIMP 

namespace settings
{
const bool SRGB = false;
const bool MIP_MAPPING = false;
const bool PIXELATED = true;
const bool VSYNC = true;
const bool MULTISAMPLING = false;
const bool SAMPLE_SHADING = true;

const bool FIXED_RATIO = false;
const bool USE_TARGET_RESOLUTION = false;
const int TARGET_WIDTH = 800;
const int TARGET_HEIGHT = 600;

#ifndef NDEBUG
const bool ERROR_ONLY = true;
#endif
}


#endif