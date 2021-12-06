#include "freeCamera.h"

namespace camera
{
	freecam::freecam(glm::vec3 position)
	{
		_position = position;
		calculateVectors();
	}

	glm::mat4 freecam::getViewMatrix()
	{
		return glm::lookAt(_position, _position + _front, _up);
	}

	float freecam::getZoom()
	{
		return _zoom;
	}


	void freecam::update(Input &input, Input &prevInput, Timer &timer)
	{
		//keyboard
		float velocity = _speed * timer.FrameElapsed();
		if(input.Keys[GLFW_KEY_W])
			_position += _front * velocity;
		if(input.Keys[GLFW_KEY_A])
			_position -= _right * velocity;
		if(input.Keys[GLFW_KEY_S])
			_position -= _front * velocity;
		if(input.Keys[GLFW_KEY_D])
			_position += _right * velocity;

		//mouse
		_pitch   += (prevInput.Y - input.Y) * _sensitivity;
		_yaw 	 += (prevInput.X - input.X) * _sensitivity;
	
		if(_pitch > 89.0f)
			_pitch = 89.0f;
		if(_pitch < -89.0f)
		 _pitch = -89.0f;
		
		//scroll
		_zoom -= input.offset * timer.FrameElapsed();
		if(_zoom < 1.0f)
			_zoom = 1.0f;
		if(_zoom > 100.0f)
			_zoom = 100.0f;

		calculateVectors();
	}
	
	void freecam::calculateVectors()
	{
		_front.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
		_front.y = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
		_front.z = sin(glm::radians(_pitch));
		_front = glm::normalize(_front);

		_right = glm::normalize(glm::cross(_front, _worldUp));
		_up = glm::normalize(glm::cross(_right, _front));
	}

} //namespace end