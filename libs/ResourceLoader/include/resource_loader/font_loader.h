#ifndef RESOURCE_FONT_LOADER
#define RESOURCE_FONT_LOADER

#include <graphics/font_loader.h>
#include <graphics/texture_loader.h>
#include <glm/glm.hpp>
#include <map>

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
    unsigned char* textureData; //caller's responsibility to free
    unsigned int width;
    unsigned int height;
    unsigned int nrChannels;
    std::map<char, Character> chars;
};

FontData* loadFont(std::string path, int fontSize);

float measureString(FontData* font, const std::string &text, float size);

std::vector<Resource::QuadDraw> getDraws(FontData *font,
					 const std::string &text, float size,
					 glm::vec2 position, float depth, glm::vec4 colour,
					 float rotate);


class InternalFontLoader : public FontLoader {
public:
    InternalFontLoader(Resource::ResourcePool pool, TextureLoader *texLoader);
    ~InternalFontLoader();
    Resource::Font LoadFont(std::string file) override;
    float MeasureString(Resource::Font font, std::string text, float size) override;

    std::vector<Resource::QuadDraw> DrawString(Resource::Font font, std::string text, glm::vec2 pos,
					       float size, float depth, glm::vec4 colour, float rotate);
    
    void clearStaged();
    void loadGPU();
    void clearGPU();
    
protected:

    void clearFonts(std::vector<FontData*> &fonts);
    
    Resource::ResourcePool pool;
    TextureLoader *texLoader;
    std::vector<FontData*> staged;
    std::vector<FontData*> fonts;
};

#endif
