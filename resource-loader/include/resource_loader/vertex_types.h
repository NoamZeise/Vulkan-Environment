#ifndef RESOURCE_LOADER_VERTEX_TYPES_H
#define RESOURCE_LOADER_VERTEX_TYPES_H

#include <glm/glm.hpp>

struct Vertex2D {
    glm::vec3 Position;
    glm::vec2 TexCoord;
};

struct Vertex3D {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
};

struct VertexAnim3D {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
    glm::ivec4 BoneIDs;
    glm::vec4  Weights;
};

#endif
