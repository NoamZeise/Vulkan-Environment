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

class Render
{
public:
	Render(GLFWwindow* window);
	void initRender(GLFWwindow* window);
	Render(GLFWwindow* window, glm::vec2 target);
	void updateProjectionMatrix();
	~Render();
	Resource::Texture LoadTexture(std::string filepath);
	Resource::Font* LoadFont(std::string filepath);
	void endTextureLoad();
	void startDraw();
	void endDraw();
	void DrawSquare(glm::vec4 drawRect, float rotate, glm::vec4 colour, uint32_t texID);
	void DrawSquare(glm::vec4 drawRect, float rotate, glm::vec4 colour, glm::vec4 textureOffsetRect, uint32_t texID);
	void DrawSquare(glm::vec4 drawRect, float rotate, uint32_t texID);
	void DrawString(Resource::Font* font, std::string text, glm::vec2 position, float size, float rotate, glm::vec4 colour);
  float MeasureString(Resource::Font* font, std::string text, float size);
	void setCameraOffset(glm::vec2 offset);
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

	bool mBegunDraw = false;
	bool mFinishedLoadingTextures = false;
	uint32_t mImg;
	VkSemaphore mImgAquireSem;
	viewProjectionBufferObj mUbo;

	void loadDataToGpu();
	void copyDataToLocalGPUMemory();
	void prepareViewProjDS();
	void prepareFragmentDescriptorSets();
	void destroySwapchainComponents();
	void resize();

#ifndef NDEBUG
	VkDebugUtilsMessengerEXT mDebugMessenger;
#endif

};





#endif
