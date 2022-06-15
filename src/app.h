#ifndef APP_H
#define APP_H

#include <atomic>
#include <iostream>
#include <thread>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>
#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <input.h>
#include <timer.h>
#include <glmhelper.h>

#include "vulkan-render/render.h"
#include "vulkan-render/resources/resources.h"
#include "vulkan-render/config.h"

#include "camera.h"

//#define TIME_APP_DRAW_UPDATE
//#define MULTI_UPDATE_ON_SLOW_DRAW

class App {
public:
  App();
  ~App();
  void run();
  void resize(int windowWidth, int windowHeight);

#pragma region GLFW_CALLBACKS
  static void framebuffer_size_callback(GLFWwindow *window, int width, int height);
  static void mouse_callback(GLFWwindow *window, double xpos, double ypos);
  static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
  static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
  static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
  static void error_callback(int error, const char *description);
#pragma endregion
  Input input;

private:
  void loadAssets();
  void update();
  void postUpdate();
  void draw();

  glm::vec2 correctedPos(glm::vec2 pos);
  glm::vec2 correctedMouse();

  const int INITIAL_WINDOW_WIDTH = 1000;
  const int INITIAL_WINDOW_HEIGHT = 700;

  GLFWwindow *mWindow;
  Render *mRender;
  int mWindowWidth, mWindowHeight;
  Input previousInput;
  Timer timer;
  Camera::FirstPerson fpcam;

  std::thread submitDraw;
  std::atomic<bool> finishedDrawSubmit;

  Resource::Model testModel;
  Resource::Model testWolf;
  Resource::Texture testTex;
  Resource::Font testFont;

  int currentBone = 0;
};

#endif
