#include "../src/render.h"
#include <graphics/glm_helper.h>

#include <exception>
#include <iostream>
#include <chrono>
#include <vector>
#include <string>

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
  if(key == GLFW_KEY_V && action == GLFW_PRESS) vsyncToggle = true;
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
    if(!glfwInit()){
      std::cerr << "Error: failed to initialise GLFW, aborting!\n";
      return -1;
    }

    // Vulkan Must be loaded before a window is created
    if(!vkenv::RenderVk::LoadVulkan()) {
	std::cerr << "Error: failed to load Vulkan!\n";
    }
    
    GLFWwindow* window = glfwCreateWindow(400, 400, "Vulkan Demo", nullptr, nullptr);
    glfwSetKeyCallback(window, key_callback);
    glfwSetErrorCallback(error_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    std::cout << "GLFW window created\n";
    RenderConfig config;
    
    try {
	vkenv::RenderVk render(window, config);
	
	std::cout << "Framebuffer Size:"
	             "\nwidth: "  << render.getTargetResolution().x
		  << "\nheight: " << render.getTargetResolution().y << std::endl;

	Resource::Texture testTex = render.LoadTexture("textures/ROOM.fbm/PolyCat.jpg");
	Resource::Model suzanneModel = render.Load3DModel("models/monkey.fbx");

	std::vector<Resource::ModelAnimation> wolfAnimations;
	Resource::Model animatedWolf = render.LoadAnimatedModel("models/wolf.fbx", &wolfAnimations);
	Resource::ModelAnimation currentWolfAnimation = wolfAnimations[0];
	Resource::ModelAnimation otherWolfAnimation = wolfAnimations[1];

	Resource::Font font = render.LoadFont("textures/Roboto-Black.ttf");
	
	render.LoadResourcesToGPU();
	render.UseLoadedResources();

	render.set2DViewMatrixAndScale(glm::mat4(1.0f), 1.0f);

	float rot = 0.0f;
	std::atomic<bool> drawFinished;
	auto start = std::chrono::high_resolution_clock::now();
	float elapsedTime = 0;
	while (!glfwWindowShouldClose(window)) {
	    glfwPollEvents();
	    if(vsyncToggle) {
		config.vsync = !config.vsync;
		render.setRenderConf(config);
		vsyncToggle = false;
	    }

	    render.set3DViewMatrixAndFov(calcView(), 45.0f, glm::vec4(camPos, 0.0f));
	    render.setTime(elapsedTime);

	    elapsedTime += frameElapsed / 1000.0f;
	    rot += 0.1f * frameElapsed;
	    
	    currentWolfAnimation.Update((float)frameElapsed);
	    otherWolfAnimation.Update((float)frameElapsed);
	
	    render.DrawQuad(testTex,
			    glmhelper::calcMatFromRect(glm::vec4(100, 240, 100, 100), rot));
	    render.DrawQuad(Resource::Texture(),
			    glmhelper::calcMatFromRect(glm::vec4(300, 240, 100, 100), -rot));

	    render.DrawString(font, "Demo", glm::vec2(10.0f, 20.0f), 10.0f, 0.0f, glm::vec4(1.0f));


	    auto monkeyMat =
			glm::rotate(
				glm::rotate(
					glm::translate(
						glm::mat4(1.0f),
						glm::vec3(100.0f, -100.0f, -100.0f)),
					glm::radians(90.0f),
					glm::vec3(1.0f, 0.0f, 0.0f)),
				glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
	    
	    auto monkeyNormalMat = glm::inverse(glm::transpose(monkeyMat));
	    render.DrawModel(suzanneModel, monkeyMat, monkeyNormalMat,
				       glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
	    monkeyMat = glm::translate(monkeyMat, glm::vec3(100.0f, 0.0f, 0.0f));
	    monkeyNormalMat = glm::inverse(glm::transpose(monkeyMat));
	    render.DrawModel(suzanneModel, monkeyMat, monkeyNormalMat,
			     glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	    monkeyMat = glm::translate(monkeyMat, glm::vec3(100.0f, 0.0f, 0.0f));
	    monkeyNormalMat = glm::inverse(glm::transpose(monkeyMat));
	    render.DrawModel(suzanneModel, monkeyMat, monkeyNormalMat,
			     glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));

	    auto wolfMat = glm::translate(
		glm::scale(
			glm::rotate(
				glm::mat4(1.0f),
				glm::radians(90.0f),
				glm::vec3(1.0f, 0.0f, 0.0f)),
			glm::vec3(2.0f, 2.0f, 2.0f)),
		glm::vec3(100.0f, -50.0f, 100.0f));
	    
	    auto wolfNormalMat = glm::inverse(glm::transpose(wolfMat));
	    render.DrawAnimModel(animatedWolf, wolfMat, wolfNormalMat, &currentWolfAnimation);
	    
	    wolfMat =
		glm::scale(
			glm::rotate(
				glm::mat4(1.0f),
				glm::radians(90.0f),
				glm::vec3(1.0f, -0.8f, 0.5f)),
			glm::vec3(2.0f, 2.0f, 2.0f)
			    );
	    wolfNormalMat = glm::inverse(glm::transpose(wolfMat));
	    render.DrawAnimModel(animatedWolf, wolfMat, wolfNormalMat, &otherWolfAnimation);
	    
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

