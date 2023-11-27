#include <resource_loader/texture_loader.h>
#include <resource_loader/stb_image.h>
#include <graphics/logger.h>

#include <stdexcept>

InternalTexLoader::InternalTexLoader(Resource::Pool pool, RenderConfig conf) {
    this->pool = pool;
    this->srgb = conf.srgb;
    this->mipmapping = conf.mip_mapping;
    this->filterNearest = conf.texture_filter_nearest;
}

InternalTexLoader::~InternalTexLoader() {
    clearStaged();
}

Resource::Texture InternalTexLoader::load(std::string path) {
    for(int i = 0; i < staged.size(); i++)
	if(staged[i].pathedTex && staged[i].path == path) {
	    return Resource::Texture(
		    i, glm::vec2(staged[i].width, staged[i].height), pool);
	}
    StagedTex tex;
    tex.path = path;
    tex.pathedTex = true;
    tex.data = stbi_load(tex.path.c_str(), &tex.width, &tex.height,
			 &tex.nrChannels, desiredChannels);
    if(!tex.data) {
	LOG_ERROR("Failed to load texture - path: " << path);
	throw std::runtime_error("texture file not found");
    }
    tex.nrChannels = desiredChannels;
    tex.filesize = tex.width * tex.height * tex.nrChannels;
    staged.push_back(tex);
    LOG("Texture Load"
	" - pool: " << pool.ID <<
	" - id: "   << staged.size() - 1 <<
	" - path: " << tex.path);
    return Resource::Texture((unsigned int)(staged.size() - 1),
			     glm::vec2(tex.width, tex.height), pool);
}

Resource::Texture InternalTexLoader::load(unsigned char* data, int width, int height, int nrChannels) {
    StagedTex tex;
    tex.data = data;
    tex.width = width;
    tex.height = height;
    tex.pathedTex = false;
    if(nrChannels != desiredChannels) {
	//TODO: CORRECT CHANNLES INSTEAD OF THROW
	throw std::runtime_error("only four channels supported");
    }
    tex.nrChannels = desiredChannels;
    tex.filesize = tex.width * tex.height * tex.nrChannels;
    staged.push_back(tex);
    LOG("Texture Load"
	" - pool: " << pool.ID <<
	" - id: "   << staged.size() - 1 <<
	" - loaded from raw data");
    return Resource::Texture(staged.size() - 1, glm::vec2(tex.width, tex.height), pool);
}

void StagedTex::deleteData() {
    if(pathedTex)
	stbi_image_free(data);
    else
	delete[] data;
}

void InternalTexLoader::clearStaged() {
    for(auto& s: staged)
	s.deleteData();
    staged.clear();
}
