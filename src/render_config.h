#ifndef VKENV_RENDER_CONFIG
#define VKENV_RENDER_CONFIG

struct RenderConfig {
  bool vsync = true;
  bool multisampling = true;
  bool srgb = false;
  bool mip_mapping = true;
  bool sample_shading = true;
  // for a pixelated look (ie no smoothing of pixels)
  bool texture_filter_nearest = false;
  bool force_target_resolution = false;
};

#endif
