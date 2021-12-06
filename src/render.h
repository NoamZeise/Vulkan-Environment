#pragma once
#ifndef RENDER_H
#define RENDER_H

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>
#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <iostream>
#include <string>
#include <cstring>

#include "vkinit.h"
#include "vkhelper.h"
#include "typeStructs.h"
#include "texture_loader.h"
#include "texfont.h"
#include "model_loader.h"

class Render
{
public:
	Render(GLFWwindow* window);
	void initRender(GLFWwindow* window);
	Render(GLFWwindow* window, glm::vec2 target);
	void setViewMatrixAndFov(glm::mat4 view, float fov);
	~Render();
	Resource::Texture LoadTexture(std::string filepath);
	Resource::Font* LoadFont(std::string filepath);
	Resource::Model LoadModel(std::string filepath);
	void endResourceLoad();
	void startDraw();
	void endDraw();
	void DrawModel(Resource::Model model, glm::mat4 modelMatrix);
	bool framebufferResized = false;
private:
	GLFWwindow* mWindow;
	glm::vec2 targetResolution;
	VkInstance mInstance;
	VkSurfaceKHR mSurface;
	Base mBase;
	FrameData mFrame;
	SwapChain mSwapchain;
	VkRenderPass mRenderPass;
	Pipeline mPipeline;
	memoryObjects mMemory;
	VkCommandPool mGeneralCommandPool;
	VkCommandBuffer mTransferCommandBuffer;
	VkDescriptorPool mDescriptorPool;
	DescriptorSets mViewprojDS;
	VkSampler mTexFragSampler;
	DescriptorSets mTexturesDS;
	Resource::TextureLoader mTextureLoader;
	Resource::ModelLoader mModelLoader;

	bool mBegunDraw = false;
	bool mFinishedLoadingResources = false;
	uint32_t mImg;
	VkSemaphore mImgAquireSem;
	viewProjectionBufferObj mUbo;
	float projectionFov = 45.0f;
	
	void updateViewProjectionMatrix();
	void prepareViewProjDS();
	void prepareFragmentDescriptorSets();
	void destroySwapchainComponents();
	void resize();

#ifndef NDEBUG
	VkDebugUtilsMessengerEXT mDebugMessenger;
#endif

};





#endif
