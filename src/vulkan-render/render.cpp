#include "render.h"

Render::Render(GLFWwindow* window)
{
	initRender(window);
	targetResolution = glm::vec2(mSwapchain.extent.width, mSwapchain.extent.height);
	updateViewProjectionMatrix();
}

Render::Render(GLFWwindow* window, glm::vec2 target)
{
	initRender(window);
	targetResolution = target;
	updateViewProjectionMatrix();
}

void Render::initRender(GLFWwindow* window)
{
	mWindow = window;
	initVulkan::instance(&mInstance);
#ifndef NDEBUG
	initVulkan::debugMessenger(mInstance, &mDebugMessenger);
#endif
	if (glfwCreateWindowSurface(mInstance, mWindow, nullptr, &mSurface) != VK_SUCCESS)
		throw std::runtime_error("failed to create window surface!");
	initVulkan::device(mInstance, mBase.physicalDevice, &mBase.device, mSurface, &mBase.queue);
	
	initFrameResources();

	//create general command pool
	VkCommandPoolCreateInfo commandPoolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	commandPoolInfo.queueFamilyIndex = mBase.queue.graphicsPresentFamilyIndex;

	if (vkCreateCommandPool(mBase.device, &commandPoolInfo, nullptr, &mGeneralCommandPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool");

	//create transfer command buffer
	VkCommandBufferAllocateInfo commandBufferInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	commandBufferInfo.commandPool = mGeneralCommandPool;
	commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(mBase.device, &commandBufferInfo, &mTransferCommandBuffer))
		throw std::runtime_error("failed to allocate command buffer");

	mModelLoader = Resource::ModelLoader(mBase, mGeneralCommandPool);
	mTextureLoader = Resource::TextureLoader(mBase, mGeneralCommandPool);
	mTextureLoader.loadTexture("textures/error.png");

	for(size_t i = 0; i < DS::MAX_BATCH_SIZE; i++)
	{
		perInstanceData.model[i] = glm::mat4(1.0f);
		perInstanceData.normalMat[i] = glm::mat4(1.0f);
	}
}

Render::~Render()
{
	vkQueueWaitIdle(mBase.queue.graphicsPresentQueue);
	mTextureLoader.~TextureLoader();
	mModelLoader.~ModelLoader();
	destroyFrameResources();
	vkDestroyCommandPool(mBase.device, mGeneralCommandPool, nullptr);
	initVulkan::destroySwapchain(&mSwapchain, mBase.device);
	vkDestroyDevice(mBase.device, nullptr);
	vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
#ifndef NDEBUG
	initVulkan::DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
#endif
	vkDestroyInstance(mInstance, nullptr);
}

void Render::initFrameResources()
{
	initVulkan::swapChain(mBase.device, mBase.physicalDevice, mSurface, &mSwapchain, mWindow, mBase.queue.graphicsPresentFamilyIndex);
	initVulkan::renderPass(mBase.device, &mRenderPass, mSwapchain);
	initVulkan::framebuffers(mBase.device, &mSwapchain, mRenderPass);

	initVulkan::CreateDescriptorSetLayout(mBase.device, &mViewprojUbo.ds,
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER}, {1}, 
		VK_SHADER_STAGE_VERTEX_BIT);
	initVulkan::CreateDescriptorSetLayout(mBase.device, &mPerInstanceSSBO.ds, 
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER}, {1}, 
		VK_SHADER_STAGE_VERTEX_BIT);
	initVulkan::CreateDescriptorSetLayout(mBase.device, &mTexturesDS, 
		{VK_DESCRIPTOR_TYPE_SAMPLER, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE}, {1, Resource::MAX_TEXTURES_SUPPORTED}, 
		VK_SHADER_STAGE_FRAGMENT_BIT);
	initVulkan::CreateDescriptorSetLayout(mBase.device, &mLightingUbo.ds,
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER}, {1},
		VK_SHADER_STAGE_FRAGMENT_BIT);

	initVulkan::graphicsPipeline(mBase.device, &mainPipeline, mSwapchain, mRenderPass, 
	{ &mViewprojUbo.ds, &mPerInstanceSSBO.ds, &mTexturesDS, &mLightingUbo.ds},
	{{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vectPushConstants)},
	{VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(vectPushConstants), sizeof(fragPushConstants)}},
	"shaders/vbasicDirectional.spv", "shaders/fBasicDirectional.spv");

	
	initVulkan::graphicsPipeline(mBase.device, &flatPipeline, mSwapchain, mRenderPass, 
	{ &mViewprojUbo.ds, &mTexturesDS, &mLightingUbo.ds},
	{{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vectPushConstants)},
	{VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(vectPushConstants), sizeof(fragPushConstants)}},
	"shaders/vflat.spv", "shaders/fflat.spv");
	
	mViewprojUbo.setPerUboProperties(mSwapchain.frameData.size(), sizeof(DS::viewProjection), DS::BufferType::Uniform);
	mPerInstanceSSBO.setPerUboProperties(mSwapchain.frameData.size(), sizeof(DS::PerInstance), DS::BufferType::Storage);
	mLightingUbo.setPerUboProperties(mSwapchain.frameData.size(), sizeof(DS::lighting), DS::BufferType::Uniform);
	vkhelper::prepareShaderBufferSets(mBase, {&mViewprojUbo, &mPerInstanceSSBO,  &mLightingUbo}, &shaderBuffer, &shaderMemory);
}

void Render::destroyFrameResources()
{
	vkDestroyBuffer(mBase.device, shaderBuffer, nullptr);
	vkFreeMemory(mBase.device, shaderMemory, nullptr);
	mViewprojUbo.ds.destroySet(mBase.device);
	mPerInstanceSSBO.ds.destroySet(mBase.device);
	mTexturesDS.destroySet(mBase.device);
	mLightingUbo.ds.destroySet(mBase.device);

	for (size_t i = 0; i < mSwapchain.frameData.size(); i++)
		vkDestroyFramebuffer(mBase.device, mSwapchain.frameData[i].framebuffer, nullptr);
	mainPipeline.destroy(mBase.device);
	flatPipeline.destroy(mBase.device);
	vkDestroyRenderPass(mBase.device, mRenderPass, nullptr);
}


Resource::Texture Render::LoadTexture(std::string filepath)
{
	if (mFinishedLoadingResources)
		throw std::runtime_error("resource loading has finished already");
	return mTextureLoader.loadTexture(filepath);
}

Resource::Font* Render::LoadFont(std::string filepath)
{
	if (mFinishedLoadingResources)
		throw std::runtime_error("resource loading has finished already");
	try
	{
		return new Resource::Font(filepath, &mTextureLoader);
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return nullptr;
	}
}

Resource::Model Render::LoadModel(std::string filepath)
{
	if(mFinishedLoadingResources)
		throw std::runtime_error("resource loading has finished already");
	return mModelLoader.loadModel(filepath, mTextureLoader);
}


void Render::endResourceLoad()
{
	mTextureLoader.endLoading();
	mModelLoader.endLoading(mTransferCommandBuffer);
	mTextureLoader.prepareFragmentDescriptorSet(mTexturesDS);
	mFinishedLoadingResources = true;
}

void Render::startDraw()
{
	if (!mFinishedLoadingResources)
		throw std::runtime_error("resource loading must be finished before drawing to screen!");
	mBegunDraw = true;
	if (mSwapchain.imageAquireSem.empty())
	{
		VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		if (vkCreateSemaphore(mBase.device, &semaphoreInfo, nullptr, &mImgAquireSem) != VK_SUCCESS)
			throw std::runtime_error("failed to create image available semaphore");
	}
	else
	{
		mImgAquireSem = mSwapchain.imageAquireSem.back();
		mSwapchain.imageAquireSem.pop_back();
	}
	if (vkAcquireNextImageKHR(mBase.device, mSwapchain.swapChain, UINT64_MAX,
		mImgAquireSem, VK_NULL_HANDLE, &mImg) != VK_SUCCESS)
	{
		mSwapchain.imageAquireSem.push_back(mImgAquireSem);
		return;
	}

	if (mSwapchain.frameData[mImg].frameFinishedFen != VK_NULL_HANDLE)
	{
		vkWaitForFences(mBase.device, 1, &mSwapchain.frameData[mImg].frameFinishedFen, VK_TRUE, UINT64_MAX);
		vkResetFences(mBase.device, 1, &mSwapchain.frameData[mImg].frameFinishedFen);
	}
	vkResetCommandPool(mBase.device, mSwapchain.frameData[mImg].commandPool, 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr; //optional - for secondary command buffers, which state to inherit from primary

	if (vkBeginCommandBuffer(mSwapchain.frameData[mImg].commandBuffer, &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to being recording command buffer");
	}

	//copy per-frame data to descriptor sets
	mViewprojUbo.storeSetData(mImg, &viewProjectionData);
	DS::lighting tempLightingData = lightingData;
	tempLightingData.direction = glm::transpose(glm::inverse(viewProjectionData.view)) * tempLightingData.direction;
	mLightingUbo.storeSetData(mImg, &tempLightingData);

	//fill render pass begin struct
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = mRenderPass;
	renderPassInfo.framebuffer = mSwapchain.frameData[mImg].framebuffer; //framebuffer for each swapchain image
												   //should match size of attachments
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = mSwapchain.extent;
	//clear colour -> values for VK_ATTACHMENT_LOAD_OP_CLEAR load operation in colour attachment
	//need colour for each attachment being cleared (colour, depth)
	std::array<VkClearValue, 2> clearColours {};
	clearColours[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearColours[1].depthStencil =  {1.0f, 0};
	renderPassInfo.clearValueCount = clearColours.size();
	renderPassInfo.pClearValues = clearColours.data();

	vkCmdBeginRenderPass(mSwapchain.frameData[mImg].commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	mModelLoader.bindBuffers(mSwapchain.frameData[mImg].commandBuffer);

	mainPipeline.begin(mSwapchain.frameData[mImg].commandBuffer, mImg);	
}

void Render::endDraw()
{
	if (!mBegunDraw)
		throw std::runtime_error("start draw before ending it");
	mBegunDraw = false;

	if(modelRuns != 0 && currentIndex < DS::MAX_BATCH_SIZE)
		drawBatch();

	mPerInstanceSSBO.storeSetData(mImg, &perInstanceData);
	currentIndex = 0;
 
	//end render pass
	vkCmdEndRenderPass(mSwapchain.frameData[mImg].commandBuffer);
	if (vkEndCommandBuffer(mSwapchain.frameData[mImg].commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}

	std::array<VkSemaphore, 1> submitWaitSemaphores = { mImgAquireSem };
	std::array<VkPipelineStageFlags, 1> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	std::array<VkSemaphore, 1> submitSignalSemaphores = { mSwapchain.frameData[mImg].presentReadySem };

	//submit draw command
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = submitWaitSemaphores.size();
	submitInfo.pWaitSemaphores = submitWaitSemaphores.data();
	submitInfo.pWaitDstStageMask = waitStages.data();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mSwapchain.frameData[mImg].commandBuffer;
	submitInfo.signalSemaphoreCount = submitSignalSemaphores.size();
	submitInfo.pSignalSemaphores = submitSignalSemaphores.data();
	if (vkQueueSubmit(mBase.queue.graphicsPresentQueue, 1, &submitInfo, mSwapchain.frameData[mImg].frameFinishedFen) != VK_SUCCESS)
		throw std::runtime_error("failed to submit draw command buffer");

	//submit present command
	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = submitSignalSemaphores.size();
	presentInfo.pWaitSemaphores = submitSignalSemaphores.data();
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &mSwapchain.swapChain;
	presentInfo.pImageIndices = &mImg;
	presentInfo.pResults = nullptr;

	VkResult result = vkQueuePresentKHR(mBase.queue.graphicsPresentQueue, &presentInfo);

	if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR || framebufferResized)
	{
		framebufferResized = false;
		resize();
	}
	else if (result != VK_SUCCESS)
		throw std::runtime_error("failed to present swapchain image to queue");

	mSwapchain.imageAquireSem.push_back(mImgAquireSem);
}

void Render::DrawModel(Resource::Model model, glm::mat4 modelMatrix, glm::mat4 normalMat)
{
	if(currentIndex >= DS::MAX_BATCH_SIZE)
	{
		std::cout << "single" << std::endl;
		vectPushConstants vps{
			modelMatrix,
			normalMat
		};   
		vps.normalMat[3][3] = 1.0;
		vkCmdPushConstants(mSwapchain.frameData[mImg].commandBuffer, mainPipeline.layout, VK_SHADER_STAGE_VERTEX_BIT,
							0, sizeof(vectPushConstants), &vps);

		mModelLoader.drawModel(mSwapchain.frameData[mImg].commandBuffer, mainPipeline.layout, model, 1, 0);
		return;
	}
	if(currentModel.ID != model.ID && modelRuns != 0)
	{
		drawBatch();
		return;
	}
	//add model to buffer
	currentModel = model;
	perInstanceData.model[currentIndex + modelRuns] = modelMatrix;
	perInstanceData.normalMat[currentIndex + modelRuns] = normalMat;
	modelRuns++;

	if(currentIndex + modelRuns == DS::MAX_BATCH_SIZE)
		drawBatch();
}

void Render::drawBatch()
{
	vectPushConstants vps{
			glm::mat4(1.0f),
			glm::mat4(0.0f)
		};   
	vkCmdPushConstants(mSwapchain.frameData[mImg].commandBuffer, mainPipeline.layout, VK_SHADER_STAGE_VERTEX_BIT,
							0, sizeof(vectPushConstants), &vps);

	mModelLoader.drawModel(mSwapchain.frameData[mImg].commandBuffer, mainPipeline.layout, currentModel, modelRuns, currentIndex);

	currentIndex += modelRuns;
	modelRuns = 0;

}

void Render::resize()
{
	vkDeviceWaitIdle(mBase.device);
	destroyFrameResources();

	initFrameResources();
	mTextureLoader.prepareFragmentDescriptorSet(mTexturesDS);

	vkDeviceWaitIdle(mBase.device);
	updateViewProjectionMatrix();
}

void Render::updateViewProjectionMatrix()
{
	float correction = 0.0f;
	float deviceRatio = mSwapchain.extent.width / mSwapchain.extent.height;
	float virtualRatio = targetResolution.x / targetResolution.y;
	float xCorrection = mSwapchain.extent.width / targetResolution.x;
	float yCorrection = mSwapchain.extent.height / targetResolution.y;

	if (virtualRatio < deviceRatio) {
		correction = yCorrection;
	}
	else {
		correction = xCorrection;
	}
	viewProjectionData.proj = glm::perspective(glm::radians(projectionFov),
			(float)mSwapchain.extent.width / (float)mSwapchain.extent.height, 0.1f, 10000.0f);
	viewProjectionData.proj[1][1] *= -1; //opengl has inversed y axis, so need to correct
}

void Render::setViewMatrixAndFov(glm::mat4 view, float fov)
{
	viewProjectionData.view = view;
	projectionFov = fov;
	updateViewProjectionMatrix();
}
