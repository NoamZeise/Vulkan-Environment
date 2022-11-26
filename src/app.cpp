#include "app.h"
#include "GLFW/glfw3.h"
#include <stdexcept>

App::App() {

  mWindowWidth = INITIAL_WINDOW_WIDTH;
  mWindowHeight = INITIAL_WINDOW_HEIGHT;

  glfwSetErrorCallback(error_callback);
  if (!glfwInit())
    throw std::runtime_error("failed to initialise glfw!");

  if(!Render::SetGLFWWindowHintsAndLoadVulkan())
      throw std::runtime_error("failed to load Vulkan! May not be supported by this device.");
  
  mWindow = glfwCreateWindow(mWindowWidth, mWindowHeight, "Vulkan App", nullptr, nullptr);
  if (!mWindow)
  {
    glfwTerminate();
    throw std::runtime_error("failed to create glfw window!");
  }

  glfwSetWindowUserPointer(mWindow, this);
  glfwSetFramebufferSizeCallback(mWindow, framebuffer_size_callback);
  glfwSetCursorPosCallback(mWindow, mouse_callback);
  glfwSetScrollCallback(mWindow, scroll_callback);
  glfwSetKeyCallback(mWindow, key_callback);
  glfwSetMouseButtonCallback(mWindow, mouse_button_callback);
  glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetInputMode(mWindow, GLFW_RAW_MOUSE_MOTION, glfwRawMouseMotionSupported());

  int width = mWindowWidth;
  int height = mWindowHeight;

  mRender = new Render(mWindow, glm::vec2(width, height));

  if (FIXED_WINDOW_RATIO)
    glfwSetWindowAspectRatio(mWindow, width, height);

  loadAssets();

  fpcam = Camera::FirstPerson(glm::vec3(3.0f, 0.0f, 2.0f));
  finishedDrawSubmit = true;
}

App::~App() {
  if (submitDraw.joinable())
    submitDraw.join();
  delete mRender;
  glfwTerminate();
}

void App::loadAssets() {
    if(current == Scene::Test1)
	loadTestScene1();
    if(current == Scene::Test2)
	loadTestScene2();
}

void App::run() {
  while (!glfwWindowShouldClose(mWindow)) {
    update();
    if (mWindowWidth != 0 && mWindowHeight != 0)
      draw();
  }
}

void App::resize(int windowWidth, int windowHeight) {
  if (submitDraw.joinable())
    submitDraw.join();
  this->mWindowWidth = windowWidth;
  this->mWindowHeight = windowHeight;
  if (mRender != nullptr && mWindowWidth != 0 && mWindowHeight != 0)
    mRender->FramebufferResize();
}

void App::update() {
#ifdef TIME_APP_DRAW_UPDATE
  auto start = std::chrono::high_resolution_clock::now();
#endif
  glfwPollEvents();

  currentWolfAnim.Update(timer);
  secondWolfAnim.Update(timer);
  thirdWolfAnim.Update(timer);

  controls();

  rotate += timer.FrameElapsed() * 0.001f;
  fpcam.update(input, previousInput, timer);

  postUpdate();
#ifdef TIME_APP_DRAW_UPDATE
  auto stop = std::chrono::high_resolution_clock::now();
  std::cout << "update: "
            << std::chrono::duration_cast<std::chrono::microseconds>(stop -
                                                                     start)
                   .count()
            << " microseconds" << std::endl;
#endif
}

void App::controls()
{
    if (input.Keys[GLFW_KEY_F] && !previousInput.Keys[GLFW_KEY_F]) {
	if (glfwGetWindowMonitor(mWindow) == nullptr) {
	    const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	    glfwSetWindowMonitor(mWindow, glfwGetPrimaryMonitor(), 0, 0, mode->width,
				 mode->height, mode->refreshRate);
	} else {
	    const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	    glfwSetWindowMonitor(mWindow, NULL, 0, 0, mWindowWidth, mWindowHeight,
				 mode->refreshRate);
	}
    }
    if (input.Keys[GLFW_KEY_ESCAPE] && !previousInput.Keys[GLFW_KEY_ESCAPE]) {
	glfwSetWindowShouldClose(mWindow, GLFW_TRUE);
    }
  const float speed = 0.001f;
  if (input.Keys[GLFW_KEY_INSERT]) {
    lightDir.x += speed * timer.FrameElapsed();
  }
  if (input.Keys[GLFW_KEY_HOME]) {
    lightDir.x -= speed * timer.FrameElapsed();
  }
  if (input.Keys[GLFW_KEY_DELETE]) {
    lightDir.z += speed * timer.FrameElapsed();
  }
  if (input.Keys[GLFW_KEY_END]) {
    lightDir.z -= speed * timer.FrameElapsed();
  }
  if (input.Keys[GLFW_KEY_PAGE_UP]) {
    lightDir.y += speed * timer.FrameElapsed();
  }
  if (input.Keys[GLFW_KEY_PAGE_DOWN]) {
    lightDir.y -= speed * timer.FrameElapsed();
  }

  if (input.Keys[GLFW_KEY_G]) {
      mRender->setTargetResolution(glm::vec2(1000, 100));
  }
  if (input.Keys[GLFW_KEY_H]) {
      mRender->setForceTargetRes(false);
  }
  if (input.Keys[GLFW_KEY_V]) {
      mRender->setVsync(true);
  }
   if (input.Keys[GLFW_KEY_B]) {
       mRender->setVsync(false);
  }

   if (input.Keys[GLFW_KEY_1]) {
       if(current != Scene::Test1) {
	   current = Scene::Test1;
	   mRender->UnloadResources();
	   this->loadAssets();
       }
  }

   if (input.Keys[GLFW_KEY_2]) {
       if(current != Scene::Test2) {
	   current = Scene::Test2;
	   mRender->UnloadResources();
	   this->loadAssets();
       }
  }

  mRender->setLightDirection(lightDir);
}

void App::postUpdate() {
  previousInput = input;
  input.offset = 0;
  timer.Update();
  mRender->set3DViewMatrixAndFov(fpcam.getViewMatrix(), fpcam.getZoom(),
                                 glm::vec4(fpcam.getPos(), 0.0));
}

void App::draw() {
#ifdef TIME_APP_DRAW_UPDATE
  auto start = std::chrono::high_resolution_clock::now();
#endif

#ifdef MULTI_UPDATE_ON_SLOW_DRAW
  if (!finishedDrawSubmit)
    return;
  finishedDrawSubmit = false;
#endif
  if (submitDraw.joinable())
    submitDraw.join();

  if(current==Scene::Test1)
      drawTestScene1();
  if(current==Scene::Test2)
      drawTestScene2();
  
  submitDraw =
    std::thread(&Render::EndDraw, mRender, std::ref(finishedDrawSubmit));



#ifdef TIME_APP_DRAW_UPDATE
  auto stop = std::chrono::high_resolution_clock::now();
  std::cout << "draw: "
            << std::chrono::duration_cast<std::chrono::microseconds>(stop -
                                                                     start)
                   .count()
            << " microseconds" << std::endl;
#endif
}

glm::vec2 App::correctedPos(glm::vec2 pos)
{
  if (mRender->isTargetResForced())
      return glm::vec2(
	pos.x * (mRender->getTargetResolution().x / (float)mWindowWidth),
	pos.y * (mRender->getTargetResolution().y / (float)mWindowHeight));

  return glm::vec2(pos.x, pos.y);
}

glm::vec2 App::correctedMouse()
{
  return correctedPos(glm::vec2(input.X, input.Y));
}

void App::loadTestScene1() {
  testModel = mRender->LoadModel("models/testScene.fbx");
  monkeyModel = mRender->LoadModel("models/monkey.obj");
  colouredCube = mRender->LoadModel("models/ROOM.fbx");
  testWolf =  mRender->LoadAnimatedModel("models/wolf.fbx", &wolfAnims);
  currentWolfAnim = wolfAnims[0];
  secondWolfAnim = wolfAnims[1];
  thirdWolfAnim = wolfAnims[4];
  testTex = mRender->LoadTexture("textures/error.png");
  testFont = mRender->LoadFont("textures/Roboto-Black.ttf");
  mRender->EndResourceLoad();
}

void App::drawTestScene1() {

  mRender->Begin3DDraw();

  auto model = glm::translate(
      glm::scale(
          glm::rotate(glm::rotate(glm::mat4(1.0f), rotate, glm::vec3(0, 0, 1)),
                      glm::radians(270.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
          glm::vec3(1.0f)),
      glm::vec3(0, 3, 0));

  mRender->DrawModel(monkeyModel, model, glm::inverseTranspose(model));

  model = glm::translate(
      glm::scale(
          glm::rotate(glm::rotate(glm::mat4(1.0f), rotate, glm::vec3(0, 0, 1)),
                      glm::radians(270.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
          glm::vec3(0.01f)),
      glm::vec3(0, 0, 0));

  mRender->DrawModel(testModel, model, glm::inverseTranspose(model));

  model = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f),
                                                glm::vec3(0.0f, -30.0f, -15.0f))

                                     ,
                                 glm::radians(270.0f),
                                 glm::vec3(-1.0f, 0.0f, 0.0f)),
                     glm::vec3(4.0f));
  mRender->DrawModel(colouredCube, model, glm::inverseTranspose(model));

  mRender->BeginAnim3DDraw();

  auto w1model = glm::translate(
      glm::scale(glm::rotate(glm::mat4(1.0f), glm::radians(270.0f),
                             glm::vec3(-1.0f, 0.0f, 0.0f)),
                 glm::vec3(0.1f)),
      glm::vec3(-100.0f, 0, 100.0f));
  mRender->DrawAnimModel(
		testWolf,
    w1model,
		glm::inverseTranspose(w1model),
    &currentWolfAnim
  );

  w1model = glm::translate(
       glm::scale(
         glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
          glm::vec3(0.1f)),
       glm::vec3(100.0f, 0, 100.0f));
  mRender->DrawAnimModel(
		testWolf,
    w1model,
		glm::inverseTranspose(w1model),
    &secondWolfAnim
  );

  w1model =  glm::translate(
       glm::scale(
         glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
          glm::vec3(0.1f)),
       glm::vec3(0.0f, 0, 100.0f));
  
  mRender->DrawAnimModel(
		testWolf,
   w1model,
		glm::inverseTranspose(w1model),
    &thirdWolfAnim
  );
      

  mRender->Begin2DDraw();

  mRender->DrawQuad(
      testTex, glmhelper::getModelMatrix(glm::vec4(400, 100, 100, 100), 0, -1),
      glm::vec4(1), glm::vec4(0, 0, 1, 1));

  mRender->DrawQuad(testTex,
                    glmhelper::getModelMatrix(glm::vec4(0, 0, 400, 400), 0, 0),
                    glm::vec4(1, 0, 1, 0.3), glm::vec4(0, 0, 1, 1));

       mRender->DrawString(testFont, "Scene 1", glm::vec2(10, 100), 40, 1.0f, glm::vec4(1), 0.0f);

}

void App::loadTestScene2() {
  monkeyModel = mRender->LoadModel("models/monkey.obj");
  colouredCube = mRender->LoadModel("models/ROOM.fbx");
  testWolf =  mRender->LoadAnimatedModel("models/wolf.fbx", &wolfAnims);
  currentWolfAnim = wolfAnims[0];
  testFont = mRender->LoadFont("textures/Roboto-Black.ttf");
  mRender->EndResourceLoad();
}

void App::drawTestScene2() {
    mRender->Begin3DDraw();
  auto model = glm::translate(
      glm::scale(
          glm::rotate(glm::rotate(glm::mat4(1.0f), rotate, glm::vec3(0, 0, 1)),
                      glm::radians(270.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
          glm::vec3(1.0f)),
      glm::vec3(0, 2, 0));

  mRender->DrawModel(monkeyModel, model, glm::inverseTranspose(model));
  model = glm::translate(
      glm::scale(
          glm::rotate(glm::rotate(glm::mat4(1.0f), rotate * 0.5f, glm::vec3(0, 0, 1)),
                      glm::radians(270.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
          glm::vec3(1.0f)),
      glm::vec3(1, 2, 0));
    mRender->DrawModel(monkeyModel, model, glm::inverseTranspose(model));
    model = glm::translate(
      glm::scale(
          glm::rotate(glm::rotate(glm::mat4(1.0f), rotate * 2.0f, glm::vec3(0, 0, 1)),
                      glm::radians(270.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
          glm::vec3(1.0f)),
      glm::vec3(2, 2, 0));
    mRender->DrawModel(monkeyModel, model, glm::inverseTranspose(model));

      model = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f),
                                                glm::vec3(0.0f, -30.0f, -15.0f))

                                     ,
                                 glm::radians(270.0f),
                                 glm::vec3(-1.0f, 0.0f, 0.0f)),
                     glm::vec3(1.0f));
      
  mRender->DrawModel(colouredCube, model, glm::inverseTranspose(model));
        model = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f),
                                                glm::vec3(0.0f, -30.0f, -15.0f))

                                     ,
                                 glm::radians(270.0f),
                                 glm::vec3(-1.0f, 0.0f, 0.0f)),
                     glm::vec3(1.0f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	
  mRender->DrawModel(colouredCube, model, glm::inverseTranspose(model));
        model = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f),
                                                glm::vec3(0.0f, -30.0f, -15.0f))

                                     ,
                                 glm::radians(270.0f),
                                 glm::vec3(-1.0f, 0.0f, 0.0f)),
                     glm::vec3(1.0f));
  mRender->DrawModel(colouredCube, model, glm::inverseTranspose(model));

    mRender->BeginAnim3DDraw();

  auto w1model = glm::translate(
      glm::scale(glm::rotate(glm::mat4(1.0f), glm::radians(270.0f),
                             glm::vec3(-1.0f, 0.0f, 0.0f)),
                 glm::vec3(0.1f)),
      glm::vec3(-50.0f, 0, 100.0f));
  mRender->DrawAnimModel(
		testWolf,
    w1model,
		glm::inverseTranspose(w1model),
    &currentWolfAnim
  );

  mRender->Begin2DDraw();

    mRender->DrawString(testFont, "Scene 2", glm::vec2(10, 100), 40, -0.4f, glm::vec4(1), 0.0f);
}



/*
 *       GLFW CALLBACKS
 */

void App::framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  auto app = reinterpret_cast<App *>(glfwGetWindowUserPointer(window));
  app->resize(width, height);
}

void App::mouse_callback(GLFWwindow *window, double xpos, double ypos) {
  App *app = reinterpret_cast<App *>(glfwGetWindowUserPointer(window));
  app->input.X = xpos;
  app->input.Y = ypos;
}
void App::scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  App *app = reinterpret_cast<App *>(glfwGetWindowUserPointer(window));
  app->input.offset = yoffset;
}

void App::key_callback(GLFWwindow *window, int key, int scancode, int action,
                       int mode) {
  App *app = reinterpret_cast<App *>(glfwGetWindowUserPointer(window));
  if (key >= 0 && key < 1024) {
    if (action == GLFW_PRESS) {
      app->input.Keys[key] = true;
    } else if (action == GLFW_RELEASE) {
      app->input.Keys[key] = false;
    }
  }
}

void App::mouse_button_callback(GLFWwindow *window, int button, int action,
                                int mods) {
  App *app = reinterpret_cast<App *>(glfwGetWindowUserPointer(window));

  if (button >= 0 && button < 8) {
    if (action == GLFW_PRESS) {
      app->input.Buttons[button] = true;
    } else if (action == GLFW_RELEASE) {
      app->input.Buttons[button] = false;
    }
  }
}

void App::error_callback(int error, const char *description) {
  throw std::runtime_error(description);
}
