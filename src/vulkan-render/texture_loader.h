#ifndef TEXTURE_LOADER_H
#define TEXTURE_LOADER_H

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include <vector>
#include <string>
#include <stdexcept>
#include <cmath>
#include <cstring>

#include "stb_image.h"
#include "config.h"
#include "render_structs.h"
#include "descriptor_sets.h"
#include "vkhelper.h"

namespace Resource
{

const int MAX_TEXTURES_SUPPORTED = 200;//match in shader

enum class TextureType
{
	Diffuse,
	Specular,
	Ambient,
	Shadow
};

struct Texture
{
	Texture()
	{
		path = "";
		ID = 0;
		dim = glm::vec2(1, 1);
		type = TextureType::Diffuse;
	}
	Texture(unsigned int ID, glm::vec2 dimentions, std::string path)
	{
		this->path = path;
		this->ID = ID;
		this->dim = dimentions;
		type = TextureType::Diffuse;
	}
	std::string path;
	unsigned int ID = 0;
	glm::vec2 dim = glm::vec2(0, 0);
	TextureType type;
};

class TextureLoader
{
public:
	TextureLoader() {};
	TextureLoader(Base base, VkCommandPool pool);
	~TextureLoader();
	void UnloadTextures();
	Texture loadTexture(std::string path);
	Texture loadTexture(unsigned char* data, int width, int height, int nrChannels);
	void endLoading();
	VkSampler* getSamplerP() { return &textureSampler; }
	VkImageView* getImageViewsP() { return &imageViews[0]; }

private:
	struct TempTexture
	{
		std::string path;
		unsigned char* pixelData;
		int width;
		int height;
		int nrChannels;
		VkFormat format;
		VkDeviceSize fileSize;
	};

	struct LoadedTexture
	{
		LoadedTexture(){}
		LoadedTexture(TempTexture tex)
		{
			width = tex.width;
			height = tex.height;
			mipLevels = (int)std::floor(std::log2(width > height ? width : height)) + 1;
		}
		uint32_t width;
		uint32_t height;
		VkImage image;
		VkImageView view;
		uint32_t mipLevels;
		VkDeviceSize imageMemSize;
	};

	VkImageView _getImageView(uint32_t texID);

	Base base;
	VkCommandPool pool;

	std::vector<TempTexture> texToLoad;
	std::vector<LoadedTexture> textures;
	VkDeviceMemory memory;
	VkImageView imageViews[MAX_TEXTURES_SUPPORTED];
	VkSampler textureSampler;
};

}
#endif
