#include <resource_loader/font_loader.h>

#include <graphics/glm_helper.h>
#include <string>
#include <stdexcept>
#include <iostream>

#include <ft2build.h>
#include FT_FREETYPE_H

const char FIRST_CHAR = ' ';
const char LAST_CHAR = '~';

struct FtLib {
    FT_Library lib;
    
    FtLib() {
	if(FT_Init_FreeType(&lib))
	    throw std::runtime_error("failed to load freetype library");
    }
    ~FtLib() {
	FT_Done_FreeType(lib);
    }
} ftlib;

struct CharData {
    unsigned char* data;
    int width;
    int height;
    Character c;
};

CharData makeChar(unsigned char* buffer, const FT_Face &face, int size);

CharData blankChar(const FT_Face &face, int size) {
    return makeChar(nullptr, face, size);
}


CharData loadChar(FT_Face face, char c, int size);

FontData* loadFont(std::string path, int fontSize) {
    FontData* fontD = new FontData();
    FT_Face face;
    if (FT_New_Face(ftlib.lib, path.c_str(), 0, &face))
	throw std::runtime_error("failed to load font at " + path);
    
    FT_Set_Pixel_Sizes(face, 0, fontSize);
    
    size_t largestHeight = 0;
    size_t totalWidth = 0;
    std::map<char, CharData> charMap;
    for (unsigned char c = FIRST_CHAR; c < LAST_CHAR; c++) {
	charMap[c] = loadChar(face, c, fontSize);
	
	totalWidth += charMap[c].width;
	largestHeight = charMap[c].height > largestHeight ?
	    charMap[c].height : largestHeight;
    }
    fontD->width = totalWidth;
    fontD->height = largestHeight;
    fontD->nrChannels = 4;
    fontD->textureData = new unsigned char[totalWidth * largestHeight * 4];
    size_t widthOffset = 0;
    for (unsigned char c = FIRST_CHAR; c < LAST_CHAR; c++) {
	for(size_t x = 0; x < charMap[c].width; x++) {
	    for(size_t y = 0; y < largestHeight; y++) {
		for (size_t byte = 0; byte < 4; byte++) {
		    if(y < charMap[c].height) {
			fontD->textureData
			    [(totalWidth*4*y) + (widthOffset*4) + (x * 4) + byte]
			    = charMap[c].data
			    [charMap[c].width * y + x];
		    } else {
			fontD->textureData[(totalWidth*4*y) + (widthOffset*4) + (x * 4) + byte]
			    = 0x00;
		    }
		}
	    }
	}
	delete[] charMap[c].data;
	charMap[c].c.texOffset  = glmhelper::getTextureOffset(
		glm::vec2(totalWidth, largestHeight),
		glm::vec4(widthOffset, 0, charMap[c].width, charMap[c].height));

	fontD->chars[c] = charMap[c].c;							     
	widthOffset += charMap[c].width;
    }

    FT_Done_Face(face);
    return fontD;
}


float measureString(FontData *font, const std::string &text, float size) {
    float sz = 0;
    for(std::string::const_iterator c = text.begin(); c != text.end(); c++) {
	if(font->chars.count(*c) == 0)
	    continue;
	sz += font->chars.at(*c).advance * size;
    }
    return sz;
}

std::vector<Resource::QuadDraw> getDraws(FontData *font,
					 const std::string &text, float size,
					 glm::vec2 position, float depth, glm::vec4 colour,
					 float rotate) {
    std::vector<Resource::QuadDraw> draws;
    for(std::string::const_iterator c = text.begin(); c != text.end(); c++) {
	if(font->chars.count(*c) == 0)
	    continue;
	Character chr = font->chars.at(*c);
	if(chr.tex.ID != 0) {
	    glm::vec4 pos = glm::vec4(position.x, position.y, 0, 0);
	    pos.x += chr.bearing.x * size;
	    pos.y += (chr.size.y - chr.bearing.y) * size;
	    pos.y -= chr.size.y * size;
	    pos.z = chr.size.x  * size;
	    pos.w = chr.size.y * size;
	    glm::mat4 model = glmhelper::calcMatFromRect(pos, rotate, depth);
	    draws.push_back(
		    Resource::QuadDraw(
			    chr.tex, model, colour, chr.texOffset));
	}
	position.x += chr.advance * size;
    }
    return draws;
}


CharData makeChar(unsigned char* buffer, const FT_Face &face, int size) {
    CharData c;
    c.data = buffer;
    c.width = face->glyph->bitmap.width;
    c.height = face->glyph->bitmap.rows;
    c.c.size = glm::vec2(c.width / (double)size, c.height / (double)size);
    c.c.bearing = glm::vec2(face->glyph->bitmap_left / (double)size,
			    face->glyph->bitmap_top / (double)size);
    c.c.advance = (float)(face->glyph->advance.x >> 6) / (double)size;
    return c;
}

CharData loadChar(FT_Face face, char c, int size) {
    CharData cd;
    if(FT_Load_Char(face, c, FT_LOAD_RENDER)) {
	std::cerr << "Error loading character: " << c << " from font"
	    ". Inserting a blank\n";
	return blankChar(face, size);
    } else if(face->glyph->bitmap.width == 0 || face->glyph->bitmap.width == 0)
	return blankChar(face, size);
    int charSize = face->glyph->bitmap.width * face->glyph->bitmap.rows;
    unsigned char* data = new unsigned char[charSize];
    memcpy(data, face->glyph->bitmap.buffer, charSize);
    return makeChar(data, face, size);
}
