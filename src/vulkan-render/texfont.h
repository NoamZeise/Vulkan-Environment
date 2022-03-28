#ifndef TEX_FONT_H
#define TEX_FONT_H

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <map>
#include <string>
#include <iostream>
#include <vector>

#include <glmhelper.h>

#include "texture_loader.h"
#include "resources.h"

namespace Resource
{

class FontLoader
{
public:
	FontLoader() {}

	~FontLoader();
	Font LoadFont(std::string file, TextureLoader* texLoader);
	std::vector<QuadDraw> DrawString(Font drawfont, std::string text, glm::vec2 position, float size, float depth, glm::vec4 colour, float rotate);
	float MeasureString(Font font, std::string text, float size);

private:
struct Character
{
	Character(Resource::Texture texture, glm::vec2 size, glm::vec2 bearing, double advance)
	{
		this->texture = texture;
		this->size = size;
		this->bearing = bearing;
		this->advance = advance;
	}
	Resource::Texture texture;
	glm::vec2 size;
	glm::vec2 bearing;
	double advance;
};

class LoadedFont
{
public:
	LoadedFont(std::string file, TextureLoader* texLoader);
	~LoadedFont();
	Character* getChar(char c);
	float MeasureString(std::string text, float size);
private:
	std::map<char, Character*> _chars;
	bool loadCharacter(TextureLoader* textureLoader, FT_Face f, char c);
	const int SIZE = 100;
};

	std::vector<LoadedFont*> fonts;
};

}
#endif
