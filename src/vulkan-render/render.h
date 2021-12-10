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
#include "pipeline.h"
#include "descriptorSets.h"
#include "texture_loader.h"
#include "texfont.h"
#include "model_loader.h"

class Render
{
public:
	Render(GLFWwindow* window);
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

	VkCommandPool mGeneralCommandPool;
	VkCommandBuffer mTransferCommandBuffer;

	Pipeline mainPipeline;
	Pipeline flatPipeline;

	//descriptor set members
	VkDeviceMemory uboMemory;
	VkBuffer uboBuffer;
	DS::UniformBufferSet mViewprojUbo;
	DS::UniformBufferSet mLightingUbo;
	DS::DescriptorSet mTexturesDS;

	DS::viewProjection viewProjectionData;
	DS::lighting lightingData;

	Resource::TextureLoader mTextureLoader;
	Resource::ModelLoader mModelLoader;

	bool mBegunDraw = false;
	bool mFinishedLoadingResources = false;
	uint32_t mImg;
	VkSemaphore mImgAquireSem;
	float projectionFov = 45.0f;
	
	void initRender(GLFWwindow* window);
	void initFrameResources();
	void destroyFrameResources();
	void resize();
	void updateViewProjectionMatrix();

#ifndef NDEBUG
	VkDebugUtilsMessengerEXT mDebugMessenger;
#endif

};





#endif
