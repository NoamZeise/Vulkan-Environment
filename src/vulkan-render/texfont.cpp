#include "texfont.h"

namespace Resource
{

Font::Font(std::string file, TextureLoader* texLoader)
{
	FT_Library ftlib;
	if (FT_Init_FreeType(&ftlib))
	{
		std::cout << "failed to load freetype2 library" << std::endl;
		return;
	}

	FT_Face face;
	if (FT_New_Face(ftlib, file.c_str(), 0, &face))
		throw std::runtime_error("failed to load font at " + file);

	FT_Set_Pixel_Sizes(face, 0, SIZE);

	for (unsigned char c = 32; c < 126; c++)
		loadCharacter(texLoader, face, c);

	FT_Done_Face(face);
	FT_Done_FreeType(ftlib);
}

Font::~Font()
{
	std::map<char, Character*>::iterator it;
	for (it = _chars.begin(); it != _chars.end(); it++)
	{
		delete it->second;
		it->second = nullptr;
	}
}

bool Font::loadCharacter(TextureLoader* textureLoader, FT_Face face, char c)
{
	if (FT_Load_Char(face, c, FT_LOAD_RENDER))
	{
		std::cout << "error loading " << c << std::endl;
		_chars.insert(std::pair<char, Character*>(c, new Character(
			Resource::Texture(0, glm::vec2(0, 0), "NULL"),
			glm::vec2(face->glyph->bitmap.width / (double)SIZE, face->glyph->bitmap.rows / (double)SIZE),
			glm::vec2(face->glyph->bitmap_left / (double)SIZE, face->glyph->bitmap_top / (double)SIZE),
			(face->glyph->advance.x >> 6) / (double)SIZE
		)));
		return true;
	}
	if (face->glyph->bitmap.width == 0)
	{
		_chars.insert(std::pair<char, Character*>(c, new Character(
			Resource::Texture(0, glm::vec2(0, 0), "NULL"),
			glm::vec2(face->glyph->bitmap.width / (double)SIZE, face->glyph->bitmap.rows / (double)SIZE),
			glm::vec2(face->glyph->bitmap_left / (double)SIZE, face->glyph->bitmap_top / (double)SIZE),
			(face->glyph->advance.x >> 6) / (double)SIZE
		)));
		return true;
	}

	unsigned char* buffer = new unsigned char[face->glyph->bitmap.width * face->glyph->bitmap.rows * 4];

	int buffIndex = 0;
	char blank = 0x00;
	for (size_t i = 0; i < face->glyph->bitmap.width * face->glyph->bitmap.rows; i++)
	{
		//std::cout << (int)((face->glyph->bitmap.buffer + i)[0]) << std::endl;
		std::memcpy(buffer + buffIndex++, face->glyph->bitmap.buffer + i, 1);
		std::memcpy(buffer + buffIndex++, face->glyph->bitmap.buffer + i, 1);
		std::memcpy(buffer + buffIndex++, face->glyph->bitmap.buffer + i, 1);
		std::memcpy(buffer + buffIndex++, face->glyph->bitmap.buffer + i, 1);
		if((char)((face->glyph->bitmap.buffer + i)[0])  != (char)0xFF)
			std::memcpy(buffer + buffIndex - 1, &blank, 1);
	}

	Resource::Texture texture = textureLoader->loadTexture(
		buffer, face->glyph->bitmap.width, face->glyph->bitmap.rows, 4);


	_chars.insert(std::pair<char, Character*>(c, new Character(
		texture,
		glm::vec2(face->glyph->bitmap.width / (double)SIZE, face->glyph->bitmap.rows / (double)SIZE),
		glm::vec2(face->glyph->bitmap_left / (double)SIZE, face->glyph->bitmap_top / (double)SIZE),
		(face->glyph->advance.x >> 6) / (double)SIZE
	)));


	return true;
}

Character* Font::getChar(char c)
{
	return _chars[c];
}

float Font::MeasureString(Resource::Font* font, std::string text, float size)
{
	float sz = 0;
	for (std::string::const_iterator c = text.begin(); c != text.end(); c++)
	{
		Resource::Character* cTex = font->getChar(*c);
		if (cTex == nullptr)
			continue;
		sz += cTex->advance * size;
	}
	return sz;
}


}
//end
