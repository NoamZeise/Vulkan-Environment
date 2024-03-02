#ifndef RENDER_GRAPHICS_RENDER_CONF_H
#define RENDER_GRAPHICS_RENDER_CONF_H

struct RenderConfig {
    bool vsync = true;
    bool multisampling = false;
    bool sample_shading = false; //can't be changed without a restart
    float target_resolution[2] = { 0.0f, 0.0f };// if [0] or [1] are zero, use resolution of window
    float depth_range_2D[2] = { 0.0f, -10.0f };
    float depth_range_3D[2] = { 0.1f, 1000.0f };
    float clear_colour[3] = { 0.39f, 0.58f, 0.93f };
    float scaled_border_colour[3] = { 0.0f, 0.0f, 0.0f };

    //Texture Loading Settings
    bool srgb = false;
    bool mip_mapping = false;
    // for a pixelated look (ie no smoothing of pixels)
    bool texture_filter_nearest = false;
};

#endif
