#ifndef MODEL_INFO_H
#define MODEL_INFO_H

#include "glm/fwd.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <vector>
#include <map>
#include <string>

namespace ModelInfo
{

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
    std::vector<unsigned int> BoneIDs;
    std::vector<float> BoneWeights;
};

struct Mesh
{
    std::vector<Vertex> verticies;
    std::vector<unsigned int> indicies;
    std::vector<std::string> diffuseTextures;
    std::vector<std::string> normalTextures;
    glm::vec4 diffuseColour;

    glm::mat4 bindTransform;
};

struct Node
{
  glm::mat4 transform;
  int parentNode;
  std::vector<int> children;
};

namespace AnimationKey
{

struct Frame
{
  double time;
};

struct Position : public Frame
{
    glm::vec3 Pos;
};

struct RotationQ : public Frame
{
  glm::quat Rot;
};

struct Scaling : public Frame
{
  glm::vec3 scale;
};
}

struct AnimNodes
{
    Node modelNode;
    int boneID = -1;
    glm::mat4 boneOffset;

    std::vector<AnimationKey::Position> positions;
    std::vector<AnimationKey::RotationQ> rotationsQ;
    std::vector<AnimationKey::Scaling> scalings;
};

struct Animation
{
    std::string name;
    double duration;
    double ticks;
    std::vector<AnimNodes> nodes;
};

struct Model
{
  std::vector<Mesh> meshes;

  std::vector<glm::mat4> bones;
  std::map<std::string, unsigned int> boneMap;

  bool animatedModel = false;
  std::vector<Animation> animations;
  std::map<std::string, unsigned int> animationMap;


  std::vector<Node> nodes;
  std::map<std::string, int> nodeMap;
};

}

#endif
