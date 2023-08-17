#ifndef TEX_FONT_H
#define TEX_FONT_H

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>

#include <string>
#include <vector>

#include <graphics/glm_helper.h>

#include "texture_loader.h"
#include <graphics/resources.h>

struct FontData;

namespace Resource
{
class FontLoader
{
public:
    FontLoader(ResourcePool resPool);
    ~FontLoader();
    Font LoadFont(std::string file, TextureLoader* texLoader);
    std::vector<QuadDraw> DrawString(Font drawfont, std::string text, glm::vec2 position, float size, float depth, glm::vec4 colour, float rotate);
    float MeasureString(Font font, std::string text, float size);
    // must be called when texture_loader.end_loading() is called
    void EndLoading();
    void UnloadStaged();
    void UnloadFonts();
    
private:
    ResourcePool resPool;
    std::vector<FontData*> fonts;
    std::vector<FontData*> stagedFonts;
};

}
#endif
