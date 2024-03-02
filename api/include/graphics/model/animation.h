/// Animated bones for a 3D model.
/// Created by model loader in graphics renderer.

#ifndef MODEL_ANIMATION_H
#define MODEL_ANIMATION_H

#include "info.h"

namespace Resource {
  class ModelAnimation {
  public:
      ModelAnimation() {}
      ModelAnimation(std::vector<glm::mat4> bones, ModelInfo::Animation animation);
      void returnToBindPose();
      /// Call this each frame to continue the animation.
      void Update(float frameElapsedMillis);
      /// get list of transforms for the all of the bones at the current point of the animation.
      std::vector<glm::mat4>* getCurrentBones() { return &bones; }
      std::string getName() { return animation.name; }
  private:
      void processNode(const ModelInfo::AnimNodes &animNode, glm::mat4 parentMat, bool animated);
      glm::mat4 boneTransform (const ModelInfo::AnimNodes &animNode);
      
      std::vector<glm::mat4> bones;
      ModelInfo::Animation animation;
      double currentTime = 0;
  };
}
#endif
