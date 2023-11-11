#ifndef RESOURCE_FONT_LOADER
#define RESOURCE_FONT_LOADER

#include <graphics/font_loader.h>
#include <graphics/texture_loader.h>
#include <glm/glm.hpp>
#include <map>

struct FontData;

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
