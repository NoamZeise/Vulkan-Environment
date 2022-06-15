#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
    std::vector<unsigned int> BoneIDs;
    std::vector<float> BoneWeights;
};

struct Mesh
{
    std::vector<Vertex> verticies;
    std::vector<unsigned int> indicies;
    std::vector<std::string> diffuseTextures;

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
struct Position
{
    glm::vec3 Pos;
    float time;
};

struct RotationQ
{
  glm::vec4 Rot;
  float time;
};

struct Scaling
{
  glm::vec3 scale;
  float time;
};
}

struct AnimNodes
{
    Node modelNode;
    int boneID = -1;

    std::vector<AnimationKey::Position> positions;
    std::vector<AnimationKey::RotationQ> rotationsQ;
    std::vector<AnimationKey::Scaling> scalings;
};

struct Animation
{
    double duration;
    double ticks;
    std::vector<AnimNodes> nodes;
};

struct Model
{
  glm::mat4 correction;
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
