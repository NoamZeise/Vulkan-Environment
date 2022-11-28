#ifndef APP_H
#define APP_H

#include <atomic>
#include <iostream>
#include <thread>

#include "vulkan-render/render.h"
#include "vulkan-render/resources/resources.h"
#include "vulkan-render/config.h"

#include <GLFW/glfw3.h>
#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <input.h>
#include <timer.h>
#include <glmhelper.h>

#include "camera.h"

//#define TIME_APP_DRAW_UPDATE
//#define MULTI_UPDATE_ON_SLOW_DRAW

const bool FIXED_WINDOW_RATIO = false;

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
  void controls();
  void postUpdate();
  void draw();

  void loadTestScene1(std::atomic<bool> &loaded);
  void drawTestScene1();
  void loadTestScene2(std::atomic<bool> &loaded);
  void drawTestScene2();

    enum class Scene {
	Test1,
	Test2,
    };
    Scene current = Scene::Test1;

  glm::vec2 correctedPos(glm::vec2 pos);
  glm::vec2 correctedMouse();

  const int INITIAL_WINDOW_WIDTH = 1000;
  const int INITIAL_WINDOW_HEIGHT = 500;

  GLFWwindow *mWindow;
  Render *mRender;
  int mWindowWidth, mWindowHeight;
  Input previousInput;
  Timer timer;
  Camera::FirstPerson fpcam;

  std::thread submitDraw;
  std::atomic<bool> finishedDrawSubmit;

  glm::vec4 lightDir = glm::vec4(0.0f, -0.5f, -1.0f, 0.0f);
  float rotate = 0.0f;

  std::thread assetLoadThread;
  std::atomic<bool> assetsLoaded;

    bool sceneChangeInProgress = false;
    
  Resource::Model testModel1;
  Resource::Model monkeyModel1;
  Resource::Model colouredCube1;
  Resource::Model testWolf1;
  std::vector<Resource::ModelAnimation> wolfAnims1;
  Resource::ModelAnimation currentWolfAnim1;
  Resource::ModelAnimation secondWolfAnim1;
  Resource::ModelAnimation thirdWolfAnim1;
  Resource::Texture testTex1;
  Resource::Font testFont1;

    Resource::Model testModel2;
  Resource::Model monkeyModel2;
  Resource::Model colouredCube2;
  Resource::Model testWolf2;
  std::vector<Resource::ModelAnimation> wolfAnims2;
  Resource::ModelAnimation currentWolfAnim2;
  Resource::ModelAnimation secondWolfAnim2;
  Resource::ModelAnimation thirdWolfAnim2;
  Resource::Texture testTex2;
  Resource::Font testFont2;
};

#endif
