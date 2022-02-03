#ifndef VULKAN_RENDERER_CONFIG_H
#define VULKAN_RENDERER_CONFIG_H


//#define NDEBUG //uncomment for release mode
//#define ONLY_2D

namespace settings
{

const bool SRGB = false;
const bool MIP_MAPPING = false;
const bool PIXELATED = false;
const bool VSYNC = false;
const bool MULTISAMPLING = false;
const bool SAMPLE_SHADING = false;

const bool FIXED_RATIO = false;
const int TARGET_WIDTH = 800;
const int TARGET_HEIGHT = 600;

#ifndef NDEBUG
const bool ERROR_ONLY = true;
#endif
}


#endif