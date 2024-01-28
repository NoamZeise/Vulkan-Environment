/// Types that represent loaded resources in the graphics renderer.
///
/// These are disposable, only have an ID and some details.
/// The actual resources are handled by the renderer.

#ifndef RESOURCES_H
#define RESOURCES_H

#include "model/animation.h"

namespace Resource {

  const uint32_t MAX_TEXTURES_SUPPORTED = 40;//match in shader
  const uint32_t MAX_2D_BATCH = 10000;
  const uint32_t MAX_3D_BATCH = 1000;
  const uint32_t MAX_BONES = 80;

  struct Pool {
      Pool() { ID = 0; }
      Pool(size_t ID) {
	  this->ID = ID;
      }
      size_t ID = 0;
  };

  struct Texture {
      Texture() {
	  ID = 0;
	  dim = glm::vec2(1, 1);
      }
      Texture(size_t ID, glm::vec2 dimentions) {
	  this->ID = ID;
	  this->dim = dimentions;
      }
      Texture(size_t ID, glm::vec2 dimentions,
	      Pool pool) {
	  this->ID = ID;
	  this->dim = dimentions;
	  this->pool = pool;
      }
      Pool pool;
      size_t ID = 0;
      glm::vec2 dim = glm::vec2(0, 0);
  };

  enum class ModelType {
      m2D,
      m3D,
      m3D_Anim,
  };
  
  struct Model {
      Model() { this->ID = 1000000;}
      Model(size_t ID) {
	  this->ID = ID;
	  this->type = ModelType::m3D;
      }
      Model(size_t ID, ModelType type, Pool pool) {
	  this->ID = ID;
	  this->type = type;
	  this->pool = pool;
      }
      Pool pool;
      ModelType type;
      size_t ID;
  };
  
  struct Font {
      Font() { this->ID = 10000000; }
      Font(size_t ID) {
	  this->ID = ID;
      }
      Font(size_t ID, Pool pool) {
	  this->ID = ID;
	  this->pool = pool;
      }
      Pool pool;
      size_t ID;
  };

  struct QuadDraw {
      QuadDraw(Texture tex, glm::mat4 model, glm::vec4 colour, glm::vec4 texOffset) {
	  this->tex = tex;
	  this->model = model;
	  this->colour = colour;
	  this->texOffset = texOffset;
      }
      QuadDraw(Texture tex, glm::mat4 model, glm::vec4 colour) {
	  this->tex = tex;
	  this->model = model;
	  this->colour = colour;
	  this->texOffset = glm::vec4(0, 0, 1, 1);
      }
      Texture tex;
      glm::mat4 model;
      glm::vec4 colour;
      glm::vec4 texOffset;
  };
}

#endif
