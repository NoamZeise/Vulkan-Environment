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

const int MAX_BONES = 50;
struct Bones {
  alignas(16) glm::mat4 mat[MAX_BONES];
};

}

#endif
