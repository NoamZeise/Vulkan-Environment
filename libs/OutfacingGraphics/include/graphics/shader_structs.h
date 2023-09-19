#ifndef GRAPHICS_SHADER_STRUCTS
#define GRAPHICS_SHADER_STRUCTS

#include <glm/glm.hpp>

struct ShaderPalette {
    alignas(16) glm::vec4 col0 = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    alignas(16) glm::vec4 col1 = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
    alignas(16) glm::vec4 col2 = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    alignas(16) glm::vec4 col3 = glm::vec4(1.0f);
};

struct Lighting3D {
  Lighting3D() {
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

#endif
