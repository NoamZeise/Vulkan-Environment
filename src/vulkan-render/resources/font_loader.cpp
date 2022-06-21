#include "font_loader.h"
#include "glmhelper.h"
#include "resources.h"

namespace Resource
{

	FontLoader::~FontLoader()
	{
		for(unsigned int i = 0; i < fonts.size(); i++)
		{
			delete fonts[i];
		}
	}

	Font FontLoader::LoadFont(std::string file, TextureLoader* texLoader)
	{
		fonts.push_back(new LoadedFont(file, texLoader));
		return Font(static_cast<unsigned int>(fonts.size() - 1));
	}

	std::vector<QuadDraw> FontLoader::DrawString(Font drawfont, std::string text, glm::vec2 position, float size, float depth, glm::vec4 colour, float rotate)
	{
		std::vector<QuadDraw> draws;
		if (drawfont.ID >= fonts.size())
		{
			std::cout << "font is out of range" << std::endl;
			return draws;
		}
		FontLoader::LoadedFont* font = fonts[drawfont.ID];
		for (std::string::const_iterator c = text.begin(); c != text.end(); c++)
		{
			Character* cTex = font->getChar(*c);
			if (cTex == nullptr)
				continue;
			else if (cTex->texture.ID != 0) //if character is added but no texture loaded for it (eg space)
			{
				glm::vec4 thisPos = glm::vec4(position.x, position.y, 0, 0);
				thisPos.x += cTex->bearing.x * size;
				thisPos.y += (cTex->size.y - cTex->bearing.y) * size;
				thisPos.y -= cTex->size.y * size;

				thisPos.z = cTex->size.x * size;
				thisPos.w = cTex->size.y * size;
				thisPos.z /= 1;
				thisPos.w /= 1;

				glm::mat4 model = glmhelper::calcMatFromRect(thisPos, rotate, depth);


				draws.push_back(QuadDraw(cTex->texture, model, colour, cTex->textureOffset));
			}
			position.x += cTex->advance * size;
		}		//test

		return draws;
	}

	float FontLoader::MeasureString(Font font, std::string text, float size)
	{
		if (font.ID >= fonts.size())
		{
			std::cout << "font is out of range" << std::endl;
			return 0.0f;
		}
		return fonts[font.ID]->MeasureString(text, size);
	}


FontLoader::LoadedFont::LoadedFont(std::string file, TextureLoader* texLoader)
{
#ifndef NDEBUG
	std::cout << "loading font: " << file << std::endl;
#endif
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

	size_t largestHeight = 0;
	size_t totalWidth = 0;
	for (unsigned char c = 32; c < 126; c++)
	{
		loadCharacter(texLoader, face, c);
		totalWidth += face->glyph->bitmap.width;
		largestHeight = face->glyph->bitmap.rows > largestHeight ? face->glyph->bitmap.rows : largestHeight;
	}

	unsigned char* finalBuff = new unsigned char[totalWidth * largestHeight * 4];
	size_t widthOffset = 0;
	for (unsigned char c = 32; c < 126; c++)
	{
		for(size_t x = 0; x < _chars[c].buffW; x++)
		{
			for(size_t y = 0; y < largestHeight; y++)
			{
				for (size_t byte = 0; byte < 4; byte++)
				{
					if(y < _chars[c].buffH)
					{
						finalBuff
							[(totalWidth*4*y) + (widthOffset*4) + (x * 4) + byte]
							= _chars[c].buffer
							[_chars[c].buffW * y + x];
					}
					else
					{
						finalBuff[(totalWidth*4*y) + (widthOffset*4) + (x * 4) + byte] = 0x00;
					}
				}
			}
		}
		delete[] _chars[c].buffer;
		_chars[c].buffer = nullptr;
		_chars[c].textureOffset  = glmhelper::getTextureOffset(
			glm::vec2(totalWidth, largestHeight),
			glm::vec4(widthOffset, 0, _chars[c].buffW, _chars[c].buffH)
		);
		widthOffset += _chars[c].buffW;
	}

	Resource::Texture texture = texLoader->LoadTexture(
		finalBuff,
		static_cast<int>(totalWidth),
		static_cast<int>(largestHeight),
		4
    );

	for (unsigned char c = 32; c < 126; c++)
	{
		_chars[c].texture = texture;
	}

	FT_Done_Face(face);
	FT_Done_FreeType(ftlib);
}

FontLoader::Character FontLoader::LoadedFont::blankChar(const FT_Face &face)
{
	return makeChar(nullptr, face);
}

FontLoader::Character FontLoader::LoadedFont::makeChar(unsigned char* buffer, const FT_Face &face)
{
	return Character(
			buffer, face->glyph->bitmap.width, face->glyph->bitmap.rows,
			glm::vec2(face->glyph->bitmap.width / (double)SIZE, face->glyph->bitmap.rows / (double)SIZE),
			glm::vec2(face->glyph->bitmap_left / (double)SIZE, face->glyph->bitmap_top / (double)SIZE),
			static_cast<float>((face->glyph->advance.x >> 6) / (double)SIZE)
	);
}

bool FontLoader::LoadedFont::loadCharacter(TextureLoader* textureLoader, FT_Face face, char c)
{
	if (FT_Load_Char(face, c, FT_LOAD_RENDER))
	{
		std::cout << "error loading " << c << std::endl;
		_chars.insert(std::pair<char, Character>(c, blankChar(face)));
		return true;
	}
	if (face->glyph->bitmap.width == 0)
	{
		_chars.insert(std::pair<char, Character>(c, blankChar(face)));
		return true;
	}

	unsigned char* buffer = new unsigned char[face->glyph->bitmap.width * face->glyph->bitmap.rows];

	for (size_t i = 0; i < face->glyph->bitmap.width * face->glyph->bitmap.rows; i++)
		buffer[i] = face->glyph->bitmap.buffer[i];

	_chars.insert(std::pair<char, Character>(c, makeChar(buffer, face)));

	return true;
}

FontLoader::Character* FontLoader::LoadedFont::getChar(char c)
{
	return &_chars[c];
}

float FontLoader::LoadedFont::MeasureString(std::string text, float size)
{
	float sz = 0;
	for (std::string::const_iterator c = text.begin(); c != text.end(); c++)
	{
		Character* cTex = getChar(*c);
		if (cTex == nullptr)
			continue;
		sz += cTex->advance * size;
	}
	return sz;
}

}
//end
