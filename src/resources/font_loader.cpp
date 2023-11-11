#include "font_loader.h"

#include <resource_loader/font_loader.h>
#include "../logger.h"

namespace Resource {

  FontLoader::FontLoader(ResourcePool resPool) {
      this->resPool = resPool;
  }
  
  FontLoader::~FontLoader() {
      UnloadStaged();
      UnloadFonts();
  }

  Font FontLoader::LoadFont(std::string file, TexLoaderVk *texLoader) {
      LOG("Loading font: " << file);
      FontData* d = loadFont(file, FONT_LOAD_SIZE);
      Resource::Texture t = texLoader->LoadTexture(d->textureData,
						   d->width,
						   d->height,
						   d->nrChannels);
      for(auto& c: d->chars)
	  c.second.tex = t;
      d->textureData = nullptr;
      stagedFonts.push_back(d);
      Font f = Font(stagedFonts.size() - 1, resPool);
      LOG("Font Loaded with ID: " << f.ID);
      return f;
  }

  void FontLoader::EndLoading() {
      UnloadFonts();
      fonts.resize(stagedFonts.size());
      for(int i = 0; i < fonts.size(); i++)
	  fonts[i] = stagedFonts[i];
      stagedFonts.clear();
  }

  void unloadFontVector(std::vector<FontData*> &fonts) {
      for(int i = 0; i < fonts.size(); i++)
	  delete fonts[i];
      fonts.clear();
  }

  void FontLoader::UnloadStaged() {
      unloadFontVector(stagedFonts);
  }

  void FontLoader::UnloadFonts() {
      unloadFontVector(fonts);
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
	  LOG_ERROR("font ID out of range in DrawString, returning no draws! ID: "
		    << drawfont.ID << " Loaded Fonts: " << fonts.size());
	  return {};
      }
      return getDraws(fonts[drawfont.ID], text, size, position, depth, colour, rotate);
  }

  
} // namespace Resource
