/// Some helper functions for using glm with the rendering framework

#ifndef GLM_HELPER_H
#define GLM_HELPER_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
	
namespace glmhelper {
    
  /// Given a section of a rect, and a section of a texture return a texture offset.
  /// A drawArea larger than the textureArea will cause the texture to repeat.
  inline glm::vec4 getTextureOffset(glm::vec4 drawArea, glm::vec4 textureArea) {
      if (drawArea.z == textureArea.z && textureArea.x == 0 && textureArea.y == 0)
	  return glm::vec4(0, 0, 1, 1);	
      glm::vec4 offset = glm::vec4(0, 0, 1, 1);
      offset.x = -(textureArea.x) / drawArea.z;
      offset.y = -(textureArea.y) / drawArea.w;
      offset.z = drawArea.z / textureArea.z;
      offset.w = drawArea.w / textureArea.w;
      return offset;
  }

  /// given the dimension of a texture and a section, return a texture offset.
  /// this allows zooming in and out of the texture on a rect.
  inline glm::vec4 getTextureOffset(glm::vec2 texDim, glm::vec4 section) {
      return glm::vec4(
	      section.x / texDim.x,
	      section.y / texDim.y,
	      section.z / texDim.x,
	      section.w / texDim.y);
  }

  /// Calculate the model matrix for a 2D rect with a rotation and a given depth.
  /// Rotation is around the center.
  inline glm::mat4 calcMatFromRect(glm::vec4 rect, float rotate, float depth) {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::mat4(1.0f);
      model = glm::translate(model, glm::vec3(rect.x, rect.y, depth));
      if(rotate != 0) {
	  model = glm::translate(model, glm::vec3(0.5 * rect.z, 0.5 * rect.w, 0.0));
	  model = glm::rotate(model, glm::radians(rotate), glm::vec3(0.0, 0.0, 1.0));
	  model = glm::translate(model, glm::vec3(-0.5 * rect.z, -0.5 * rect.w, 0.0));
      }
      model = glm::scale(model, glm::vec3(rect.z, rect.w, 1.0f));
      return model;
  }

  /// Calculate the model matrix for a 2D rect with a rotation.
  /// Rotation is around the center.
  /// Depth is set to 0.
  inline glm::mat4 calcMatFromRect(glm::vec4 rect, float rotate) {
      return calcMatFromRect(rect, rotate, 0.0);
  }

  inline glm::mat4 calcFinalOffset(glm::vec2 offscreenRes, glm::vec2 finalRes) {
      float ratio = (offscreenRes.x / offscreenRes.y) *
	  (finalRes.y / finalRes.x);
      return glm::scale(
	      glm::mat4(1.0f),
	      glm::vec3(ratio < 1.0f ? ratio: 1.0f,
			ratio > 1.0f ? 1.0f / ratio : 1.0f,
			1.0f));
  }
}
#endif
