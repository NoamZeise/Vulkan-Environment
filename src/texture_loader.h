#pragma once
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
#include "typeStructs.h"
#include "vkhelper.h"

const int MAX_TEXTURES_SUPPORTED = 200;//match in shader

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

struct Texture
{
	Texture(){}
	Texture(TempTexture tex)
	{
		width = tex.width;
		height = tex.height;
		mipLevels = std::floor(std::log2(width > height ? width : height)) + 1;
	}
	uint32_t width;
	uint32_t height;
	VkImage image;
	VkImageView view;
	uint32_t mipLevels;
	VkDeviceSize imageMemSize;
};

class TextureLoader
{
public:
	TextureLoader() {};
	TextureLoader(Base base, VkCommandPool pool);
	~TextureLoader();
	uint32_t loadTexture(std::string path);
	uint32_t loadTexture(unsigned char* data, int width, int height, int nrChannels);
	VkImageView getImageView(uint32_t texID);
	void endLoading();

	VkSampler sampler;

private:
	Base base;
	VkCommandPool pool;

	std::vector<TempTexture> texToLoad;
	std::vector<Texture> textures;
	VkDeviceMemory memory;
};






#endif
