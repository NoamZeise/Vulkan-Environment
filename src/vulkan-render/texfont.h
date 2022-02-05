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
#include <stdexcept>
#include <cstring>

#include "texture_loader.h"

namespace Resource
{

struct Character
{
	Character(Resource::Texture texture, glm::vec2 size, glm::vec2 bearing, double advance)
	{
		this->texture = texture;
		this->size = size;
		this->bearing = bearing;
		this->advance = advance;
	}
	~Character()
	{

	}
	Resource::Texture texture;
	glm::vec2 size;
	glm::vec2 bearing;
	double advance;
};

class Font
{
public:
	Font(std::string file, TextureLoader* texLoader);
	~Font();
	Character* getChar(char c);
	static float MeasureString(Resource::Font* font, std::string text, float size);
private:
	std::map<char, Character*> _chars;
	bool loadCharacter(TextureLoader* textureLoader, FT_Face f, char c);
	const int SIZE = 100;
};

}
#endif
