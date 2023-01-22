#ifndef TEX_FONT_H
#define TEX_FONT_H

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>

#include <string>
#include <vector>

#include <glmhelper.h>

#include "texture_loader.h"
#include <resources/resources.h>

namespace Resource
{

    class LoadedFont;

class FontLoader
{
public:
    FontLoader() {}
    ~FontLoader();
    Font LoadFont(std::string file, TextureLoader* texLoader);
    std::vector<QuadDraw> DrawString(Font drawfont, std::string text, glm::vec2 position, float size, float depth, glm::vec4 colour, float rotate);
    float MeasureString(Font font, std::string text, float size);
    void UnloadFonts();
    
private:
	std::vector<LoadedFont*> fonts;
};

}
#endif
