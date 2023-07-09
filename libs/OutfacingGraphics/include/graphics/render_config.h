#ifndef RENDER_GRAPHICS_RENDER_CONF_H
#define RENDER_GRAPHICS_RENDER_CONF_H

struct RenderConfig {
    bool vsync = true;
    bool multisampling = true;
    bool srgb = false;
    bool mip_mapping = true;
    bool sample_shading = true;
    // for a pixelated look (ie no smoothing of pixels)
    bool texture_filter_nearest = false;
    bool force_target_resolution = false;
    float depth_range_2D[2] = { -10.0f, 10.0f };
    float depth_range_3D[2] = { 0.1f, 1000.0f };
    float clearColour[3] = {0.2f, 0.6f, 0.6f};
};

#endif
