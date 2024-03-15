/// Types that represent loaded resources in the graphics renderer.
///
/// These are disposable, only have an ID and some details.
/// The actual resources are handled by the renderer.

#ifndef RESOURCES_H
#define RESOURCES_H

#include "model/animation.h"

namespace Resource {

  const uint32_t MAX_TEXTURES_SUPPORTED = 20;//match in shader
  const uint32_t MAX_2D_BATCH = 10000;
  const uint32_t MAX_3D_BATCH = 1000;
  const uint32_t MAX_BONES = 80;

  static size_t NULL_POOL_ID = SIZE_MAX;

  struct Pool {
      Pool() { }
      Pool(size_t ID) {
	  this->ID = ID;
      }
      bool operator==(Pool other) {
	  return ID == other.ID;
      }
      bool operator!=(Pool other) {
	  return !(*this == other);
      }
      
      size_t ID = NULL_POOL_ID;
  };

  static size_t NULL_ID = SIZE_MAX;
  
  struct Texture {
      Texture() {
	  ID = 0;
	  dim = glm::vec2(1, 1);
      }
      Texture(size_t ID) {
	  this->ID = ID;
      }
      Texture(size_t ID, glm::vec2 dimentions) {
	  this->ID = ID;
	  this->dim = dimentions;
      }
      Texture(size_t ID, glm::vec2 dimentions, Pool pool) {
	  this->ID = ID;
	  this->dim = dimentions;
	  this->pool = pool;
      }

      bool operator==(Texture other) {
	  return
	      pool == other.pool &&
	      ID == other.ID &&
	      dim == other.dim;
      }
      bool operator!=(Texture other) {
	  return !(*this == other);
      }
      
      Pool pool;
      size_t ID = NULL_ID;
      glm::vec2 dim = glm::vec2(0, 0);
  };

  enum class ModelType {
      m2D,
      m3D,
      m3D_Anim,
  };

  static size_t NULL_MODEL_ID = SIZE_MAX;
  
  struct Model {
      Model() {}
      Model(size_t ID) {
	  this->ID = ID;
	  this->type = ModelType::m3D;
      }
      Model(size_t ID, ModelType type, Pool pool) {
	  this->ID = ID;
	  this->type = type;
	  this->pool = pool;
      }

      bool operator==(Model other) {
	  return
	      ID == other.ID &&
	      type == other.type &&
	      pool == other.pool &&
	      overrideTexture == other.overrideTexture &&
	      colour.r == other.colour.r &&
	      colour.g == other.colour.g &&
	      colour.b == other.colour.b &&
	      colour.a == other.colour.a;
      }
      bool operator!=(Model other) {
	  return !(*this == other);
      }
      
      
      Pool pool;
      ModelType type;
      Resource::Texture overrideTexture = Resource::Texture(NULL_ID);
      // use diffuse colour if alpha == 0
      glm::vec4 colour = glm::vec4(0);
      size_t ID = NULL_MODEL_ID;
  };

  static size_t NULL_FONT_ID = SIZE_MAX;
  
  struct Font {
      Font() {}
      Font(size_t ID) {
	  this->ID = ID;
      }
      Font(size_t ID, Pool pool) {
	  this->ID = ID;
	  this->pool = pool;
      }
      bool operator==(Font other) {
	  return pool == other.pool &&
	      ID == other.ID;
      }
      
      Pool pool;
      size_t ID = NULL_FONT_ID;
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
      bool operator==(QuadDraw other) {
	  return tex == other.tex &&
	      model == other.model &&
	      colour == other.colour &&
	      texOffset == other.texOffset;
      }
      Texture tex;
      glm::mat4 model;
      glm::vec4 colour;
      glm::vec4 texOffset;
  };
}

#endif
