#ifndef FREE_CAMERA_H
#define FREE_CAMERA_H

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>
#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#include <iostream>

#include <input.h>
#include <timer.h>

namespace Camera
{
	class FirstPerson
	{

	public:
		FirstPerson() { _position = glm::vec3(0.0f, 0.0f, 0.0f); };
		FirstPerson(glm::vec3 position);
		glm::mat4 getViewMatrix();
		float getZoom();
		void update(Input &input, Input &prevInput, Timer &timer);


	private:
		glm::vec3 _position;
		glm::vec3 _front;
		glm::vec3 _up;
		glm::vec3 _right;
		glm::vec3 _worldUp = glm::vec3(0.0f, 0.0f, 1.0f);
		glm::mat4 view = glm::mat4(1.0f);
		bool viewUpdated = true;

		float _yaw = 200.0f;
		float _pitch = -20.0f;

		float _speed = 0.01f;
		float _sensitivity = 0.05f;
		float _zoom = 45.0f;


		void calculateVectors();
	};


}	//namesapce end



#endif
