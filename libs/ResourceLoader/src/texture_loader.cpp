#include <resource_loader/texture_loader.h>

TextureLoader::TextureLoader(Resource::ResourcePool pool, RenderConfig conf) {
    this->pool = pool;
    this->srgb = conf.srgb;
    this->mipmapping = conf.mip_mapping;
    this->filterNearest = conf.texture_filter_nearest;
}
