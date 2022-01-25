#ifndef VULKAN_RENDERER_CONFIG_H
#define VULKAN_RENDERER_CONFIG_H


//#define NDEBUG //uncomment for release mode

namespace settings
{

const bool SRGB = false;
const bool MIP_MAPPING = true;
const bool PIXELATED = true;
const bool VSYNC = true;
const bool MULTISAMPLING = true;
const bool SAMPLE_SHADING = true;

const bool FIXED_RATIO = false;
const int TARGET_WIDTH = 800;
const int TARGET_HEIGHT = 600;

#ifndef NDEBUG
const bool ERROR_ONLY = true;
#endif
}


#endif