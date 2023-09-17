#ifndef GRAPHICS_SHADER_STRUCTS
#define GRAPHICS_SHADER_STRUCTS

#include <glm/glm.hpp>

struct ShaderPalette {
    alignas(16) glm::vec4 col0 = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    alignas(16) glm::vec4 col1 = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
    alignas(16) glm::vec4 col2 = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    alignas(16) glm::vec4 col3 = glm::vec4(1.0f);    
};

#endif
