#include <graphics/model/animation.h>
#include <graphics/logger.h>

namespace Resource {
  
  ModelAnimation::ModelAnimation(std::vector<glm::mat4> bones, ModelInfo::Animation animation) {
      this->bones = bones;
      this->animation = animation;
      returnToBindPose();
  }

  void ModelAnimation::returnToBindPose() {
      processNode(animation.nodes[0], glm::mat4(1.0f), false);
  }
  
  void ModelAnimation::Update(float frameElapsesdMillis) {
      currentTime = fmod(currentTime + (frameElapsesdMillis * animation.ticks), animation.duration);
      processNode(animation.nodes[0], glm::mat4(1.0f), true);
  }
  
  void ModelAnimation::processNode(const ModelInfo::AnimNodes &animNode, glm::mat4 parentMat, bool animated){
      glm::mat4 nodeMat = animNode.modelNode.transform;
      if(animNode.modelNode.boneID != -1) {
	  if(animated)
	      nodeMat = boneTransform(animNode);
	  bones[animNode.modelNode.boneID] =
	      parentMat * nodeMat * animNode.modelNode.boneOffset;	  
      }
      nodeMat = parentMat * nodeMat;
      for(const auto& childID: animNode.modelNode.children)
	  processNode(animation.nodes[childID], nodeMat, animated);
  }

  template <typename T>
  glm::mat4 bone(const std::vector<T> &frames, float currentTime);
  
  glm::mat4 ModelAnimation::boneTransform(const ModelInfo::AnimNodes &animNode) {
      glm::mat4 mat =
	  bone(animNode.positions, currentTime)
	  * bone(animNode.rotationsQ, currentTime)
	  * bone(animNode.scalings, currentTime);
      if(mat == glm::mat4(1.0f))
	  return animNode.modelNode.transform;
      else
	  return mat;
  }
  
  /// --- helpers ---
  
  struct FrameProps {
      int f1 = 0;   // frame 1
      int f2 = 0;   // frame 2
      double r = 0; // factor
  };
  
  double calcFactor(double t1, double t2, double currentTime) {
      return (currentTime - t1) / (t2 - t1);
  }

  float getTime(ModelInfo::AnimationKey::Frame frame) { return frame.time; }
  
  template <typename T>
  FrameProps interpFrames(const std::vector<T> &frames, float currentTime) {
      if(frames.size() == 1)
	  return FrameProps{0, 0, 0};      
      int second = 0;
      for(int i = 0 ; i < frames.size(); i++) {
	  if(getTime(frames[i]) >= currentTime) {
	      second = i;
	      break;
	  }
      }
      int first = (second - 1) % frames.size();
      double factor = calcFactor(getTime(frames[first]), getTime(frames[second]), currentTime);
      return FrameProps{first, second, factor};
  }

  glm::mat4 frameMat(const std::vector<ModelInfo::AnimationKey::Position> &frames, FrameProps s) {
      return glm::translate(
	      glm::mat4(1.0f),
	      glm::mix(frames[s.f1].Pos, frames[s.f2].Pos, s.r));
  }
  
  glm::mat4 frameMat(const std::vector<ModelInfo::AnimationKey::RotationQ> &frames, FrameProps s) {
      glm::quat rot = glm::slerp(frames[s.f1].Rot, frames[s.f2].Rot, (float)s.r);
      return glm::toMat4(glm::normalize(rot));
  }

  glm::mat4 frameMat(const std::vector<ModelInfo::AnimationKey::Scaling> &frames, FrameProps s) {
      glm::vec3 scale = glm::mix(frames[s.f1].scale, frames[s.f2].scale, s.r);
      return glm::scale(glm::mat4(1.0f), scale);
  }
  
  template <typename T>
  glm::mat4 bone(const std::vector<T> &frames, float currentTime) {
      if(frames.size() == 0)
	  return glm::mat4(1.0f);
      auto s = interpFrames(frames, currentTime);
      return frameMat(frames, s);
  }
}
