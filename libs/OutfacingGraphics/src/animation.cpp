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

  glm::mat4 ModelAnimation::boneTransform(const ModelInfo::AnimNodes &animNode) {
      glm::mat4 mat =
	  bonePos(animNode.positions) * boneRot(animNode.rotationsQ) * boneScl(animNode.scalings);
      if(mat == glm::mat4(1.0f))
	  return animNode.modelNode.transform;
      else
	  return mat;
  }

  //TODO: make less dry 
  
  glm::mat4 ModelAnimation::bonePos(
	  const std::vector<ModelInfo::AnimationKey::Position> &posFrames) {
      if(posFrames.size() == 0)
	  return glm::mat4(1.0f);
      if(posFrames.size() == 1 || false)
	  return glm::translate(glm::mat4(1.0f), posFrames[0].Pos);
      
      int second = 0;
      for(int i = 0 ; i < posFrames.size(); i++) {
	  if(posFrames[i].time >= currentTime) {
	      second = i;
	      break;
	  }
      }
      int first = (second - 1) % posFrames.size();
      
      double factor = getFactor(posFrames[first].time, posFrames[second].time);
      return glm::translate(
	      glm::mat4(1.0f),
	      glm::mix(posFrames[first].Pos, posFrames[second].Pos, factor));
  }

  glm::mat4 ModelAnimation::boneRot(
	  const std::vector<ModelInfo::AnimationKey::RotationQ> &rotFrames) {
      if(rotFrames.size() == 0)
	  return glm::mat4(1.0f);
      if(rotFrames.size() == 1)
	  return glm::toMat4(glm::normalize(rotFrames[0].Rot));
      int second = 0;
      for(int i = 0 ; i < rotFrames.size(); i++) {
	  if(rotFrames[i].time >= currentTime) {
	      second = i;
	      break;
	  }
      }
      int first = (second - 1) % rotFrames.size();

      float factor = getFactor(rotFrames[first].time, rotFrames[second].time);
      glm::quat rot = glm::slerp(rotFrames[first].Rot, rotFrames[second].Rot, factor);
      return glm::toMat4(glm::normalize(rot));
  }

  glm::mat4 ModelAnimation::boneScl(
	  const std::vector<ModelInfo::AnimationKey::Scaling> &sclFrames) {
      if(sclFrames.size() == 0)
	  return glm::mat4(1.0f);
      if(sclFrames.size() == 1)
	  return glm::scale(glm::mat4(1.0f), sclFrames[0].scale);
      
      int second = 0;
      for(int i = 0 ; i < sclFrames.size(); i++) {
	  if(sclFrames[i].time >= currentTime) {
	      second = i;
	      break;
	  }
      }
      int first = (second - 1) % sclFrames.size();
      double factor = getFactor(sclFrames[first].time, sclFrames[second].time);
      glm::vec3 scale = glm::mix(sclFrames[first].scale, sclFrames[second].scale, factor);
      return glm::scale(glm::mat4(1.0f), scale);
  }
}
