/// A format for storing models that can be loaded by the graphics framework.
/// This does not have to be used, and the graphics framework can load models directly from files.

#ifndef MODEL_INFO_H
#define MODEL_INFO_H

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <vector>
#include <map>
#include <string>

namespace ModelInfo {
  /// A point on the model.
  /// 2D models will only use position and texcoord.
  /// 3D models will only use position, texcoord and normal.
  /// 3D animated models will use all of these properties.
  struct Vertex {
      glm::vec3 Position;
      glm::vec3 Normal;
      glm::vec2 TexCoord;
      std::vector<unsigned int> BoneIDs;
      std::vector<float> BoneWeights;  
      Vertex() {
	  Position = {0.0f, 0.0f, 0.0f};
	  Normal = {0.0f, 0.0f, 0.0f};
	  TexCoord = {0.0f, 0.0f};
      }
  };

  /// A collection of verticies with indicies, color and a bind transform.
  struct Mesh {
      std::vector<Vertex> verticies;
      /// indicies index into the vertex array, to build triangles, which allows for vertex reuse
      /// when mutliple triangles share a vertex.
      std::vector<unsigned int> indices;
      /// Diffuse textures change the look of the model.
      std::vector<std::string> diffuseTextures;
      /// The diffuse color gives the overall color of the model,
      /// or tint if using diffuse textures too.
      glm::vec4 diffuseColour;
      /// The bind transform is the natural transform of the model without the influence of animations.
      /// Leave as the identity matrix if the model is not animated.
      glm::mat4 bindTransform;
      Mesh() {
	  diffuseColour = glm::vec4(1.0f);
	  bindTransform = glm::mat4(1.0f);
      }
  };

  /// node transform tree of the model. stores the relative positions of meshes in the model.
  struct Node {
      glm::mat4 transform;
      int parentNode;
      std::vector<int> children;
      int boneID = -1;
      glm::mat4 boneOffset;
  };

  /// Key frames of the state of the model's bones
  namespace AnimationKey {
    struct Frame {
	double time;
    };
    
    struct Position : public Frame {
	glm::vec3 Pos;
    };
    
    struct RotationQ : public Frame {
	glm::quat Rot;
    };
    
    struct Scaling : public Frame {
	glm::vec3 scale;
    };
  }

  /// a frame of animation for all of the bones.
  struct AnimNodes {
      Node modelNode;
      std::vector<AnimationKey::Position> positions;
      std::vector<AnimationKey::RotationQ> rotationsQ;
      std::vector<AnimationKey::Scaling> scalings;
  };

  /// An animation for a 3D model.
  struct Animation {
      std::string name;
      double duration;
      double ticks;
      std::vector<AnimNodes> nodes;
  };

  // A model, made of a collection of meshes, with bones and animations if animated, and a node map.
  struct Model {
      std::vector<Mesh> meshes;
      std::vector<Node> nodes;
      
      std::map<std::string, int> nodeMap;
      std::vector<glm::mat4> bones;
      std::map<std::string, unsigned int> boneMap;
      std::vector<Animation> animations;
  };
}

#endif
