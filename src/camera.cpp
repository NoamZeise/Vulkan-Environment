#include "camera.h"

namespace Camera
{
	FirstPerson::FirstPerson(glm::vec3 position)
	{
		_position = position;
		calculateVectors();
	}

	glm::mat4 FirstPerson::getViewMatrix()
	{
		if(viewUpdated)
		{
			view = glm::lookAt(_position, _position + _front, _up);
			viewUpdated = false;
			return view;
		}
		return view;
	}

	float FirstPerson::getZoom()
	{
		return _zoom;
	}


	void FirstPerson::update(Input &input, Input &prevInput, Timer &timer)
	{
		viewUpdated = true;
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
		if(input.Keys[GLFW_KEY_SPACE])
			_position += _worldUp * velocity;
		if(input.Keys[GLFW_KEY_LEFT_SHIFT])
			_position -= _worldUp * velocity;

		//mouse
		_pitch   += (float)(prevInput.Y - input.Y) * _sensitivity;
		_yaw 	 += (float)(prevInput.X - input.X) * _sensitivity;

		if(_pitch > 89.0f)
			_pitch = 89.0f;
		if(_pitch < -89.0f)
		 _pitch = -89.0f;

		//scroll
		_zoom -= (float)input.offset * timer.FrameElapsed();
		if(_zoom < 1.0f)
			_zoom = 1.0f;
		if(_zoom > 100.0f)
			_zoom = 100.0f;

		calculateVectors();
		//std::cout << "X:" << _position.x << " Y:" << _position.y << " Z:" << _position.z << std::endl;
	}

	void FirstPerson::calculateVectors()
	{
		_front.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
		_front.y = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
		_front.z = sin(glm::radians(_pitch));
		_front = glm::normalize(_front);

		_right = glm::normalize(glm::cross(_front, _worldUp));
		_up = glm::normalize(glm::cross(_right, _front));
	}

} //namespace end
