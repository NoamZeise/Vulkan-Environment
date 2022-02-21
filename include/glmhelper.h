#ifndef GLM_HELPER_H
#define GLM_HELPER_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
	
namespace glmhelper
{

inline glm::mat4 getModelMatrix(glm::vec4 drawRect, float rotate, float zpos)
{
	glm::mat4 model = glm::mat4(1.0f);

	model = glm::translate(model, glm::vec3(drawRect.x, drawRect.y, zpos)); //translate object by position
	//rotate object
	model = glm::translate(model, glm::vec3(0.5 * drawRect.z, 0.5 * drawRect.w, 0.0)); // move object by half its size, so rotates around centre
	model = glm::rotate(model, glm::radians(rotate), glm::vec3(0.0, 0.0, 1.0));//then do rotation
	model = glm::translate(model, glm::vec3(-0.5 * drawRect.z, -0.5 * drawRect.w, 0.0)); //then translate back to original position

	model = glm::scale(model, glm::vec3(drawRect.z, drawRect.w, 1.0f)); //then scale

	return model;
}

inline glm::mat4 getModelMatrix(glm::vec4 drawRect, float rotate)
{
	return getModelMatrix(drawRect, rotate, 0.0f);
}

inline glm::vec4 getTextureOffset(glm::vec4 drawArea, glm::vec4 textureArea)
{
	if (drawArea.z == textureArea.z && textureArea.x == 0 && textureArea.y == 0)
		return glm::vec4(0, 0, 1, 1);

	glm::vec4 offset = glm::vec4(0, 0, 1, 1);
	offset.x = -(textureArea.x) / drawArea.z;
	offset.y = -(textureArea.y) / drawArea.w;
	offset.z = drawArea.z / textureArea.z;
	offset.w = drawArea.w / textureArea.w;

	return offset;
}

inline glm::mat4 calcMatFromRect(glm::vec4 rect, float rotate, float depth)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(rect.x, rect.y, depth));
	if(rotate != 0)
	{
		model = glm::translate(model, glm::vec3(0.5 * rect.z, 0.5 * rect.w, 0.0));
		model = glm::rotate(model, glm::radians(rotate), glm::vec3(0.0, 0.0, 1.0));
		model = glm::translate(model, glm::vec3(-0.5 * rect.z, -0.5 * rect.w, 0.0));
	}
	model = glm::scale(model, glm::vec3(rect.z, rect.w, 1.0f));
	return model;
}

inline glm::mat4 calcMatFromRect(glm::vec4 rect, float rotate)
{
	return calcMatFromRect(rect, rotate, 0.0);
}

}//namespace end

#endif