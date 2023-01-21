#include "../src/render.h"
#include "glmhelper.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <chrono>


glm::vec3 camPos = glm::vec3(-300.0f, 10.0f, 0.0f);
float yaw = -5.0f;
float pitch = 2.0f;
    
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    if (key == GLFW_KEY_W) pitch += 1.0f;
    if (key == GLFW_KEY_S) pitch -= 1.0f;
    if (key == GLFW_KEY_A) yaw += 1.0f;
    if (key == GLFW_KEY_D) yaw -= 1.0f;

    if(key == GLFW_KEY_UP) camPos.x += 1.0f;
    if(key == GLFW_KEY_DOWN) camPos.x -= 1.0f;
    if(key == GLFW_KEY_LEFT) camPos.y += 1.0f;
    if(key == GLFW_KEY_RIGHT) camPos.y -= 1.0f;
    if(key == GLFW_KEY_SPACE) camPos.z += 1.0f;
    if(key == GLFW_KEY_LEFT_SHIFT) camPos.z -= 1.0f;
}

void error_callback(int error, const char *description) {
  throw std::runtime_error(description);
}

glm::mat4 calcView() {
    glm::vec3 front = glm::normalize(glm::vec3(
				     cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
				     sin(glm::radians(yaw)) * cos(glm::radians(pitch)),
				     sin(glm::radians(pitch)))
				     );

    return glm::lookAt(camPos, camPos + front, glm::normalize(glm::cross(glm::normalize(glm::cross(front, glm::vec3(0.0f, 0.0f, 1.0f))), front)));
}


int main() {
    std::cout << "--- Vulkan Environment Demo ---\n";
    if(!glfwInit())
    {
	std::cerr << "Error: failed to initialise GLFW, aborting!\n";
	return -1;
    }

    // Vulkan Must be loaded before a window is created
    if(!vkenv::Render::LoadVulkan()) {
	std::cerr << "Error: failed to load Vulkan!\n";
    }
    
    GLFWwindow* window = glfwCreateWindow(400, 400, "Vulkan Demo", nullptr, nullptr);
    glfwSetKeyCallback(window, key_callback);
    glfwSetErrorCallback(error_callback);
    
    vkenv::Render render(window, glm::vec2(400, 400));

    std::cout << "Framebuffer Size:\nwidth: " << render.getTargetResolution().x << "\nheight: " << render.getTargetResolution().y << std::endl;;

    Resource::Texture testTex = render.LoadTexture("textures/ROOM.fbm/PolyCat.jpg");
    Resource::Model suzanneModel = render.LoadModel("models/monkey.fbx");

    render.LoadResourcesToGPU();
    render.UseLoadedResources();

    render.set2DViewMatrixAndScale(glm::mat4(1.0f), 1.0f);

    float rot = 0.0f;
    std::atomic<bool> drawFinished;
    auto start = std::chrono::high_resolution_clock::now();
    long frameElapsed = 0;
    while (!glfwWindowShouldClose(window)) {
	glfwPollEvents();

    render.set3DViewMatrixAndFov(calcView(), 45.0f, glm::vec4(camPos, 0.0f));
	rot += 0.1f * frameElapsed;

	render.Begin2DDraw();
	
	render.DrawQuad(testTex, glmhelper::calcMatFromRect(glm::vec4(100, 100, 100, 100), rot));

	render.Begin3DDraw();

	auto model =
	    glm::rotate(
			glm::rotate(
				    glm::mat4(1.0f),
				    glm::radians(90.0f),
				    glm::vec3(1.0f, 0.0f, 0.0f)),
			glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f)
			);
	render.DrawModel(suzanneModel, model, glm::inverse(glm::transpose(model)));
	
	drawFinished = false;
	render.EndDraw(drawFinished);

	frameElapsed = std::chrono::duration_cast<std::chrono::milliseconds>
	    (std::chrono::high_resolution_clock::now() - start).count();
	start =  std::chrono::high_resolution_clock::now();
    }
    
    glfwDestroyWindow(window);
    return 0;
}

