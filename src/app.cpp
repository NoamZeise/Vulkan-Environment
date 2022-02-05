#include "app.h"

App::App()
{
	//set member variables
	mWindowWidth = 800;
	mWindowHeight = 450;
	//init glfw window
	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
			throw std::runtime_error("failed to initialise glfw!");
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //using vulkan not openGL
	mWindow = glfwCreateWindow(mWindowWidth, mWindowHeight, "Vulkan App", nullptr, nullptr);
	if(!mWindow)
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
	if (glfwRawMouseMotionSupported())
    	glfwSetInputMode(mWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	
	if(FIXED_RATIO)
		glfwSetWindowAspectRatio(mWindow, TARGET_WIDTH, TARGET_HEIGHT);
		
	mRender = new Render(mWindow, glm::vec2(TARGET_WIDTH, TARGET_HEIGHT));
		loadAssets();
	freecam = camera::firstPerson(glm::vec3(3.0f, 0.0f, 2.0f));

}

App::~App()
{
	if(submitDraw.joinable())
		submitDraw.join();
	delete testFont;
	delete mRender;
	mRender = nullptr;
	glfwDestroyWindow(mWindow);
	glfwTerminate();
}

void App::loadAssets()
{
	testModel = mRender->LoadModel("models/testScene.fbx");
	testTex = mRender->LoadTexture("textures/error.png");
	testFont = mRender->LoadFont("textures/Roboto-Black.ttf");
	threeChannelTest = mRender->LoadTexture("textures/error.jpg");
	mRender->endResourceLoad();
}

void App::run()
{
	while (!glfwWindowShouldClose(mWindow))
	{
		update();
		if(mWindowWidth != 0 && mWindowHeight != 0)
			draw();
	}
}

void App::resize(int windowWidth, int windowHeight)
{
	if(submitDraw.joinable())
		submitDraw.join();
	mWindowWidth = windowWidth;
	mWindowHeight = windowHeight;
	if(mRender != nullptr && mWindowWidth != 0 && mWindowHeight != 0)
		mRender->framebufferResize();
}

void App::update()
{
#ifdef TIME_APP_DRAW_UPDATE
	auto start = std::chrono::high_resolution_clock::now();
#endif
	glfwPollEvents();



	postUpdate();
#ifdef TIME_APP_DRAW_UPDATE
	auto stop = std::chrono::high_resolution_clock::now();
	std::cout
		 << "update: "
         << std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count()
		 << " microseconds" << std::endl;
#endif
}

void App::postUpdate()
{
	if (input.Keys[GLFW_KEY_F] && !previousInput.Keys[GLFW_KEY_F])
	{
		if (glfwGetWindowMonitor(mWindow) == nullptr)
		{
			const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
			glfwSetWindowMonitor(mWindow, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
		}
		else
		{
			const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
			glfwSetWindowMonitor(mWindow, NULL, 100, 100, mWindowWidth, mWindowHeight, mode->refreshRate);
		}
	}

	if(input.Keys[GLFW_KEY_ESCAPE] && !previousInput.Keys[GLFW_KEY_ESCAPE])
	{
		glfwSetWindowShouldClose(mWindow, GLFW_TRUE);
	}

	time += timer.FrameElapsed();
	freecam.update(input, previousInput, timer);
	timer.Update();
	previousInput = input;
	input.offset = 0;
	mRender->setViewMatrixAndFov(freecam.getViewMatrix(), freecam.getZoom());
}


void App::draw()
{
#ifdef TIME_APP_DRAW_UPDATE
	auto start = std::chrono::high_resolution_clock::now();
#endif

#ifdef MULTI_UPDATE_ON_SLOW_DRAW
	if(!finishedDrawSubmit)
		return;
	finishedDrawSubmit = false;
#endif
	if(submitDraw.joinable())
		submitDraw.join();

	mRender->begin2DDraw();


	mRender->begin3DDraw();

	mRender->DrawModel(testModel, glm::mat4(1.0f), glm::inverseTranspose(freecam.getViewMatrix() * glm::mat4(1.0f)));

	mRender->begin2DDraw();

	mRender->DrawQuad(testTex, glmhelper::getModelMatrix(glm::vec4(250, 250, 500, 500), 0), glm::vec4(1, 0, 0, 0.5), glm::vec4(-0.5f,-0.5f, 1, 1));
	mRender->DrawQuad(testTex, glmhelper::getModelMatrix(glm::vec4(0, 0, 500, 500), 0), glm::vec4(0, 1, 1, 0.5), glm::vec4(0, 0, 1, 1));

	mRender->DrawQuad(threeChannelTest, glmhelper::getModelMatrix(glm::vec4(1050, 750, 400, 400), 0), glm::vec4(1, 1, 1, 0.8));

	mRender->DrawString(testFont, "text on the screen", glm::vec2(100, 100), 70, 0, glm::vec4(1, 1, 1, 1));

	//end Draw
	submitDraw = std::thread(&Render::endDraw, mRender, std::ref(finishedDrawSubmit));

#ifdef TIME_APP_DRAW_UPDATE
	auto stop = std::chrono::high_resolution_clock::now();
	std::cout
	<< "draw: "
	<< std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count()
	<< " microseconds" << std::endl;
#endif
}

glm::vec2 App::correctedPos(glm::vec2 pos)
{
	return glm::vec2(pos.x * ((float)TARGET_WIDTH / (float)mWindowWidth), pos.y * ((float)TARGET_HEIGHT / (float)mWindowHeight));
}

glm::vec2 App::correctedMouse()
{
	return correctedPos(glm::vec2(input.X, input.Y));
}

#pragma region GLFW_CALLBACKS


void App::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
	app->resize(width, height);
}

void App::mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	App* app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
	app->input.X = xpos;
	app->input.Y = ypos;
}
void App::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	App* app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
	app->input.offset = yoffset;
}

void App::key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	App* app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			app->input.Keys[key] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			app->input.Keys[key] = false;
		}
	}
}

void App::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	App* app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));

	if (button >= 0 && button < 8)
	{
		if (action == GLFW_PRESS)
		{
			app->input.Buttons[button] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			app->input.Buttons[button] = true;
		}
	}
}

void App::error_callback(int error, const char* description)
{
    throw std::runtime_error(description);
}

#pragma endregion
