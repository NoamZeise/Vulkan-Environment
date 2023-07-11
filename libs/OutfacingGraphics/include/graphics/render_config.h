#ifndef RENDER_GRAPHICS_RENDER_CONF_H
#define RENDER_GRAPHICS_RENDER_CONF_H

#include <glm/glm.hpp>

struct RenderConfig {
    bool vsync = true;
    bool multisampling = false;
    bool srgb = false;
    bool mip_mapping = false;
    bool sample_shading = false;
    // for a pixelated look (ie no smoothing of pixels)
    bool texture_filter_nearest = false;
    bool force_target_resolution = false;
    float depth_range_2D[2] = { -10.0f, 10.0f };
    float depth_range_3D[2] = { 0.1f, 1000.0f };
    float clear_colour[3] = { 0.39f, 0.58f, 0.93f };
    float scaled_boarder_colour[3] = { 0.0f, 0.0f, 0.0f };
};

#endif
