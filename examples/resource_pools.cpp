#include "../src/render.h"
#include <graphics/glm_helper.h>

#include <exception>
#include <iostream>
#include <chrono>
#include <vector>
#include <string>

const int WIN_WIDTH = 400;
const int WIN_HEIGHT = 400;

glm::vec3 camPos = glm::vec3(-350.0f, 10.0f, 0.0f);
float yaw = -5.0f;
float pitch = -5.0f;
bool resize = false;
bool vsyncToggle = false;
long frameElapsed = 0;

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    resize = true;
}
    
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  float speed = 0.2f;
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  if (key == GLFW_KEY_W) pitch += speed * frameElapsed;
  if (key == GLFW_KEY_S) pitch -= speed * frameElapsed;
  if (key == GLFW_KEY_A) yaw += speed * frameElapsed;
  if (key == GLFW_KEY_D) yaw -= speed * frameElapsed;
  speed = 0.5f;
  if(key == GLFW_KEY_UP) camPos.x += speed * frameElapsed;
  if(key == GLFW_KEY_DOWN) camPos.x -= speed * frameElapsed;
  if(key == GLFW_KEY_LEFT) camPos.y += speed * frameElapsed;
  if(key == GLFW_KEY_RIGHT) camPos.y -= speed * frameElapsed;
  if(key == GLFW_KEY_SPACE) camPos.z += speed * frameElapsed;
  if(key == GLFW_KEY_LEFT_SHIFT) camPos.z -= speed * frameElapsed;
}

void error_callback(int error, const char *description) {
  throw std::runtime_error(description);
}

glm::mat4 calcView() {
    glm::vec3 front = glm::normalize(glm::vec3(
				     cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
				     sin(glm::radians(yaw)) * cos(glm::radians(pitch)),
				     sin(glm::radians(pitch))));

    return glm::lookAt(camPos, camPos + front,
		       glm::normalize(glm::cross(glm::normalize(glm::cross(
			     front, glm::vec3(0.0f, 0.0f, 1.0f))), front)));
}

int main() {
    std::cout << "--- Vulkan Environment Demo ---\n";
    if(!glfwInit()){
      std::cerr << "Error: failed to initialise GLFW!\n";
      return 1;
    }
    if(!vkenv::RenderVk::LoadVulkan()) {
	std::cerr << "Error: failed to load Vulkan!\n";
	return 1;
    }
    
    GLFWwindow* window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT,
					  "Resource Pools", nullptr, nullptr);
    glfwSetKeyCallback(window, key_callback);
    glfwSetErrorCallback(error_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    std::cout << "GLFW window created\n";
    RenderConfig config;
    
    try {
	vkenv::RenderVk render(window, config);

	Resource::ResourcePool pool1 = render.CreateResourcePool();
	Resource::ResourcePool pool2 = render.CreateResourcePool();

	Resource::Texture texture1 = render.LoadTexture(pool1, "textures/ROOM.fbm/PolyCat.jpg");
	Resource::Texture texture2 = render.LoadTexture(pool2, "textures/ROOM.fbm/PolyHearts.jpg");
	
	Resource::Font font = render.LoadFont("textures/Roboto-Black.ttf");
	
	render.LoadResourcesToGPU();
	render.LoadResourcesToGPU(pool1);
	render.LoadResourcesToGPU(pool2);
	render.UseLoadedResources();

	render.set2DViewMatrixAndScale(glm::mat4(1.0f), 1.0f);

	float rot = 0.0f;
	std::atomic<bool> drawFinished;
	auto start = std::chrono::high_resolution_clock::now();
	float elapsedTime = 0;
	while (!glfwWindowShouldClose(window)) {
	    glfwPollEvents();

	    render.set3DViewMatrixAndFov(calcView(), 45.0f, glm::vec4(camPos, 0.0f));

	    elapsedTime += frameElapsed / 1000.0f;
	    rot += 0.1f * frameElapsed;

	    render.DrawQuad(texture1,
			    glmhelper::calcMatFromRect(glm::vec4(100, 240, 100, 100), rot));
	    render.DrawQuad(texture2,
			    glmhelper::calcMatFromRect(glm::vec4(300, 240, 100, 100), -rot));
	    
	    render.DrawString(font, "Resource Pool Demo",
			      glm::vec2(10.0f, 20.0f), 10.0f, 0.0f, glm::vec4(1.0f));

	    drawFinished = false;
	    render.EndDraw(drawFinished);

	    if(resize) {	
		resize = false;
		render.FramebufferResize();
	    }

	    frameElapsed = (long)std::chrono::duration_cast<std::chrono::milliseconds>
		(std::chrono::high_resolution_clock::now() - start).count();
	    start =  std::chrono::high_resolution_clock::now();
	}
    }
    catch (const std::exception &e) {
	std::cerr << "Exception occured during render: " << e.what() << std::endl;
    }

    glfwDestroyWindow(window);
    return 0;
}

