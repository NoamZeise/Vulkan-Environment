/// Collection of types used in the default shaders for the rendering framework.

#ifndef VKENV_SHADER_STRUCTS_H
#define VKENV_SHADER_STRUCTS_H

#include "glm/glm.hpp"

namespace shaderStructs {
struct viewProjection {
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

struct PerFrame3D {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 normalMat;
};

struct Frag2DData {
  alignas(16) glm::vec4 colour;
  alignas(16) glm::vec4 texOffset;
  alignas(4) uint32_t texID;
};

struct timeUbo {
    alignas(4) float time;
};

struct Lighting {
  Lighting() {
    ambient = glm::vec4(1.0f, 1.0f, 1.0f, 0.35f);
    diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 0.8f);
    specular = glm::vec4(1.0f, 1.0f, 1.0f, 8.0f);
    direction = glm::vec4(0.3f, 0.3f, -0.5f, 0.0f);
  }

  alignas(16) glm::vec4 ambient;
  alignas(16) glm::vec4 diffuse;
  alignas(16) glm::vec4 specular;
  alignas(16) glm::vec4 direction;
  alignas(16) glm::vec4 camPos;
};

const int MAX_BONES = 50;
struct Bones {
  alignas(16) glm::mat4 mat[MAX_BONES];
};

}

#endif
