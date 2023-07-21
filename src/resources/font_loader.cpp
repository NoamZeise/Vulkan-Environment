#include "font_loader.h"

#ifndef NO_FREETYPE

#include <resource_loader/font_loader.h>
#include "../logger.h"

namespace Resource {

  FontLoader::~FontLoader() {
      UnloadFonts();
  }

  Font FontLoader::LoadFont(std::string file, TextureLoader *texLoader) {
      LOG("Loading font: " << file << " ID: " << fonts.size());
      FontData* d = loadFont(file, FONT_LOAD_SIZE);
      Resource::Texture t = texLoader->LoadTexture(d->textureData,
						   d->width,
						   d->height,
						   d->nrChannels);
      for(auto& c: d->chars)
	  c.second.tex = t;
      d->textureData = nullptr;
      fonts.push_back(d);
      return Font((unsigned int)(fonts.size() - 1));
  }

  void FontLoader::UnloadFonts() {
      for(int i = 0; i < fonts.size(); i++)
	  delete fonts[i];
      fonts.clear();
  }

float FontLoader::MeasureString(Font font, std::string text, float size) {
    if (font.ID >= fonts.size()) {
	LOG_ERROR("font ID was out of range in MeasureString, returning 0.");
	return 0.0f;
    }
    return measureString(fonts[font.ID], text, size);
}
  
  std::vector<QuadDraw> FontLoader::DrawString(Font drawfont, std::string text,
					       glm::vec2 position, float size,
					       float depth, glm::vec4 colour, float rotate) {
      if (drawfont.ID >= fonts.size()) {
	  LOG_ERROR("font ID out of range in DrawString, returning no draws!");
	  return {};
      }
      return getDraws(fonts[drawfont.ID], text, size, position, depth, colour, rotate);
  }

  
} // namespace Resource



#else


#include <stdexcept>

namespace Resource {
    FontLoader::~FontLoader() {}
    Font FontLoader::LoadFont(std::string file, TextureLoader* texLoader) { throw std::runtime_error("Tried to use Font::LoadFont, but NO_FREETYPE is defined"); }
    std::vector<QuadDraw> FontLoader::DrawString(Font drawfont, std::string text, glm::vec2 position, float size, float depth, glm::vec4 colour, float rotate) { throw std::runtime_error("Tried to use Font::DrawString, but NO_FREETYPE is defined"); }
    float FontLoader::MeasureString(Font font, std::string text, float size) { throw std::runtime_error("Tried to use Font::MesaureString, but NO_FREETYPE is defined"); }
    void UnloadFonts() {}
}

#endif
//end
