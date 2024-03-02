#include <resource_loader/font_loader.h>

#include <graphics/glm_helper.h>
#include <graphics/logger.h>
#include <stdexcept>

const int FONT_LOAD_SIZE = 100;

struct Character {
    Resource::Texture tex;
    bool blank = false;
    glm::vec4 texOffset;
    glm::vec2 size;
    glm::vec2 bearing;
    float advance;
};

struct FontData {
    unsigned char* textureData;
    unsigned int width;
    unsigned int height;
    unsigned int nrChannels;
    std::map<char, Character> chars;
};

InternalFontLoader::InternalFontLoader(Resource::Pool pool, TextureLoader * texLoader) {
    this->pool = pool;
    this->texLoader = texLoader;
}

InternalFontLoader::~InternalFontLoader() {
    clearStaged();
    clearGPU();
}

FontData* loadFont(std::string path, int fontSize);

Resource::Font InternalFontLoader::load(std::string file) {
    FontData* d = loadFont(file, FONT_LOAD_SIZE);
    Resource::Texture t = texLoader->load(d->textureData,
					  d->width,
					  d->height,
					  d->nrChannels);
    d->textureData = nullptr; // ownership taken by texloader
    for(auto& c: d->chars)
	c.second.tex = t;
    staged.push_back(d);
    Resource::Font f(staged.size() - 1, pool);
    LOG("Font Loaded - pool: " << pool.ID <<
	" - id: " << f.ID <<
	" - path: " << file);
    return f;
}

void InternalFontLoader::loadGPU() {
    clearGPU();
    fonts = staged;
    staged.clear();
}

void InternalFontLoader::clearFonts(std::vector<FontData *> &fonts) {
    for(int i = 0; i < fonts.size(); i++)
	delete fonts[i];
    fonts.clear();
}

void InternalFontLoader::clearGPU() { clearFonts(fonts); }

void InternalFontLoader::clearStaged() { clearFonts(staged); }

float InternalFontLoader::length(Resource::Font font, std::string text, float size) {
    if(font.ID >= fonts.size()) {
	LOG_ERROR("font ID: " << font.ID << " was out of range: " << fonts.size());
	return 0.0f;
    }
    float sz = 0;
    for(std::string::const_iterator c = text.begin(); c != text.end(); c++) {
	if(fonts[font.ID]->chars.count(*c) == 0)
	    continue;
	sz += fonts[font.ID]->chars.at(*c).advance * size;
    }
    return sz;
}

std::vector<Resource::QuadDraw> InternalFontLoader::DrawString(Resource::Font font,
							       std::string text,
							       glm::vec2 pos,
							       float size,
							       float depth,
							       glm::vec4 colour,
							       float rotate) {
    if(font.ID >= fonts.size()) {
	LOG_ERROR("font ID: " << font.ID << " was out of range: " << fonts.size());
	return {};
    }
    std::vector<Resource::QuadDraw> draws;
    for(std::string::const_iterator c = text.begin(); c != text.end(); c++) {
	if(fonts[font.ID]->chars.count(*c) == 0)
	    continue;
	Character chr = fonts[font.ID]->chars.at(*c);
	if(!chr.blank) {
	    glm::vec4 p = glm::vec4(pos.x, pos.y, 0, 0);
	    p.x += chr.bearing.x * size;
	    p.y += (chr.size.y - chr.bearing.y) * size;
	    p.y -= chr.size.y * size;
	    p.z = chr.size.x  * size;
	    p.w = chr.size.y * size;
	    glm::mat4 model = glmhelper::calcMatFromRect(p, rotate, depth);
	    draws.push_back(
		    Resource::QuadDraw(
			    chr.tex, model, colour, chr.texOffset));
	}
	pos.x += chr.advance * size;
    }
    return draws;
}



/// ---- Freetype Font Loader ----

#ifdef NO_FREETYPE

#include <stdexcept>
FontData* loadFont(std::string path, int fontSize) {
    throw std::runtime_error("Tried to load font, but application "
			     "was build without the freetype library");
}

#else

#include <ft2build.h>
#include FT_FREETYPE_H

const char FIRST_CHAR = ' ';
const char LAST_CHAR = '~';
const size_t CHANNELS = 4;

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
    size_t spacing = 2;
    std::map<char, CharData> charMap;
    for (unsigned char c = FIRST_CHAR; c < LAST_CHAR; c++) {
	charMap[c] = loadChar(face, c, fontSize);
	
	totalWidth += charMap[c].width + spacing;
	largestHeight = charMap[c].height > largestHeight ?
	    charMap[c].height : largestHeight;
    }
    fontD->width = totalWidth;
    fontD->height = largestHeight;
    fontD->nrChannels = CHANNELS;
    fontD->textureData = new unsigned char[totalWidth * largestHeight * fontD->nrChannels];
    size_t widthOffset = 0;
    for (unsigned char c = FIRST_CHAR; c < LAST_CHAR; c++) {
	for(size_t x = 0; x < charMap[c].width + spacing; x++) {
	    for(size_t y = 0; y < largestHeight; y++) {
		for (size_t byte = 0; byte < fontD->nrChannels; byte++) {
		    fontD->textureData[(totalWidth*fontD->nrChannels*y)
				       + (widthOffset*fontD->nrChannels)
				       + (x * fontD->nrChannels) + byte]
			= y < charMap[c].height && x < charMap[c].width
			?
			(byte == fontD->nrChannels - 1 ? //last channel has the rendered colour
			 charMap[c].data[charMap[c].width * y + x]
			 : 0xFF)
			: 0x00;
		}
	    }
	}
	delete[] charMap[c].data;
	charMap[c].c.texOffset = glmhelper::getTextureOffset(
		glm::vec2(totalWidth, largestHeight),
		glm::vec4(widthOffset, 0, charMap[c].width, charMap[c].height));

	fontD->chars[c] = charMap[c].c;							     
	widthOffset += charMap[c].width + spacing;
    }

    FT_Done_Face(face);
    return fontD;
}


CharData makeChar(unsigned char* buffer, const FT_Face &face, int size) {
    CharData c;
    c.data = buffer;
    c.width = face->glyph->bitmap.width;
    c.height = face->glyph->bitmap.rows;
    if(buffer == nullptr)
	c.c.blank = true;
    c.c.size = glm::vec2(
	    c.width / (float)size,
	    c.height / (float)size);
    c.c.bearing = glm::vec2(
	    face->glyph->bitmap_left / (float)size,
	    face->glyph->bitmap_top / (float)size);
    c.c.advance = (float)(face->glyph->advance.x >> 6) / (float)size;
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
#endif
