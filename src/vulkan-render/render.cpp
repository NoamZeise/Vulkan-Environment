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

	initVulkan::CreateDescriptorSetLayout(mBase.device, &mViewproj3DUbo.ds,
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER}, {1}, 
		VK_SHADER_STAGE_VERTEX_BIT);
	initVulkan::CreateDescriptorSetLayout(mBase.device, &mViewproj2DUbo.ds,
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

	initVulkan::graphicsPipeline(mBase.device, &pipeline3D, mSwapchain, mRenderPass, 
			{ &mViewproj3DUbo.ds, &mPerInstanceSSBO.ds, &mTexturesDS, &mLightingUbo.ds},
			{{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vectPushConstants)},
			{VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(vectPushConstants), sizeof(fragPushConstants)}},
			"shaders/v3D-lighting.spv", "shaders/fblinnphong.spv", true);

	initVulkan::graphicsPipeline(mBase.device, &pipeline2D, mSwapchain, mRenderPass, 
			{ &mViewproj2DUbo.ds, &mPerInstanceSSBO.ds, &mTexturesDS,},
			{{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vectPushConstants)},
			{VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(vectPushConstants), sizeof(fragPushConstants)}},
			"shaders/vflat.spv", "shaders/fflat.spv", false);
	
	mViewproj3DUbo.setPerUboProperties(mSwapchain.frameData.size(), sizeof(DS::viewProjection), DS::BufferType::Uniform);
	mViewproj2DUbo.setPerUboProperties(mSwapchain.frameData.size(), sizeof(DS::viewProjection), DS::BufferType::Uniform);
	mPerInstanceSSBO.setPerUboProperties(mSwapchain.frameData.size(), sizeof(DS::PerInstance), DS::BufferType::Storage);
	mLightingUbo.setPerUboProperties(mSwapchain.frameData.size(), sizeof(DS::lighting), DS::BufferType::Uniform);
	vkhelper::prepareShaderBufferSets(mBase, {&mViewproj3DUbo, &mViewproj2DUbo,  &mPerInstanceSSBO,  &mLightingUbo}, &shaderBuffer, &shaderMemory);
	mTextureLoader.prepareFragmentDescriptorSet(mTexturesDS, mSwapchain.frameData.size());
}

void Render::destroyFrameResources()
{
	vkDestroyBuffer(mBase.device, shaderBuffer, nullptr);
	vkFreeMemory(mBase.device, shaderMemory, nullptr);
	mViewproj3DUbo.ds.destroySet(mBase.device);
	mViewproj2DUbo.ds.destroySet(mBase.device);
	mPerInstanceSSBO.ds.destroySet(mBase.device);
	mTexturesDS.destroySet(mBase.device);
	mLightingUbo.ds.destroySet(mBase.device);
	for (size_t i = 0; i < mSwapchain.frameData.size(); i++)
		vkDestroyFramebuffer(mBase.device, mSwapchain.frameData[i].framebuffer, nullptr);
	pipeline3D.destroy(mBase.device);
	pipeline2D.destroy(mBase.device);
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
	mFinishedLoadingResources = true;
	mTextureLoader.endLoading();
	mModelLoader.endLoading(mTransferCommandBuffer);
	initFrameResources();
}

void Render::resize()
{
	vkDeviceWaitIdle(mBase.device);
	destroyFrameResources();

	initFrameResources();

	vkDeviceWaitIdle(mBase.device);
	updateViewProjectionMatrix();
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
	beginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(mSwapchain.frameData[mImg].commandBuffer, &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to being recording command buffer");
	}

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
}

void Render::begin3DDraw()
{
	if(!mBegunDraw)
		startDraw();
	if(modelRuns > 0)
		drawBatch();
	m3DRender = true; 
		
	mViewproj3DUbo.storeSetData(mImg, &viewProjectionData3D);
	DS::lighting tempLightingData = lightingData;
	tempLightingData.direction = glm::transpose(glm::inverse(viewProjectionData3D.view)) * tempLightingData.direction;
	mLightingUbo.storeSetData(mImg, &tempLightingData);

	pipeline3D.begin(mSwapchain.frameData[mImg].commandBuffer, mImg);	
}

void Render::begin2DDraw()
{
	if(!mBegunDraw)
		startDraw();
	if(modelRuns > 0)
		drawBatch();
	m3DRender = false;

	float correction;
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
	viewProjectionData2D.proj = glm::ortho(0.0f, (float)mSwapchain.extent.width / correction, 0.0f, (float)mSwapchain.extent.height / correction, -1.0f, 1.0f);
	viewProjectionData2D.view = glm::mat4(1.0f);

	mViewproj2DUbo.storeSetData(mImg, &viewProjectionData2D);

	pipeline2D.begin(mSwapchain.frameData[mImg].commandBuffer, mImg);
}

void Render::endDraw(std::atomic<bool>& submit)
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

	//most of draw call time spent here!
	VkResult result = vkQueuePresentKHR(mBase.queue.graphicsPresentQueue, &presentInfo);

	if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR || framebufferResized)
	{
		framebufferResized = false;
		resize();
	}
	else if (result != VK_SUCCESS)
		throw std::runtime_error("failed to present swapchain image to queue");

	mSwapchain.imageAquireSem.push_back(mImgAquireSem);

	submit = true;
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
		vkCmdPushConstants(mSwapchain.frameData[mImg].commandBuffer, pipeline3D.layout, VK_SHADER_STAGE_VERTEX_BIT,
							0, sizeof(vectPushConstants), &vps);

		mModelLoader.drawModel(mSwapchain.frameData[mImg].commandBuffer, pipeline3D.layout, model, 1, 0);
		return;
	}
	
	if(currentModel.ID != model.ID && modelRuns != 0)
		drawBatch();
	//add model to buffer
	currentModel = model;
	perInstanceData.model[currentIndex + modelRuns] = modelMatrix;
	perInstanceData.normalMat[currentIndex + modelRuns] = normalMat;
	modelRuns++;

	if(currentIndex + modelRuns == DS::MAX_BATCH_SIZE)
		drawBatch();
}

void Render::DrawQuad(const Resource::Texture& texture, glm::mat4 modelMatrix, glm::vec4 colour, glm::vec4 texOffset)
{
	if(currentIndex >= DS::MAX_BATCH_SIZE)
	{
		std::cout << "single" << std::endl;

		vectPushConstants vps{
			modelMatrix,
			glm::mat4(1.0f)
		};   
		
		vps.normalMat[3][3] = 1.0;
		vkCmdPushConstants(mSwapchain.frameData[mImg].commandBuffer, pipeline3D.layout, VK_SHADER_STAGE_VERTEX_BIT,
							0, sizeof(vectPushConstants), &vps);

		mModelLoader.drawQuad(mSwapchain.frameData[mImg].commandBuffer, pipeline3D.layout, texture.ID,
		 	1, 0, colour, texOffset);
		return;
	}

	if( modelRuns != 0 && 
		(currentTexture.ID != currentTexture.ID ||
		 currentTexOffset != texOffset ||
		 currentColour    != colour))
	{
		drawBatch();
	}

	//add model to buffer
	currentTexture = texture;
	currentColour = colour;
	currentTexOffset = texOffset;
	perInstanceData.model[currentIndex + modelRuns] = modelMatrix;
	modelRuns++;

	if(currentIndex + modelRuns == DS::MAX_BATCH_SIZE)
		drawBatch();
}

void Render::DrawString(Resource::Font* font, std::string text, glm::vec2 position, float size, float rotate, glm::vec4 colour)
{
	if (font == nullptr)
	{
		std::cout << "font is NULL" << std::endl;
		return;
	}
	for (std::string::const_iterator c = text.begin(); c != text.end(); c++)
	{
		Resource::Character* cTex = font->getChar(*c);
		if (cTex == nullptr)
			continue;
		else if (cTex->TextureID != 0) //if character is added but no texture loaded for it (eg space)
		{
			glm::vec4 thisPos = glm::vec4(position.x, position.y, 0, 0);
			thisPos.x += cTex->Bearing.x * size;
			thisPos.y += (cTex->Size.y - cTex->Bearing.y) * size;
			thisPos.y -= cTex->Size.y * size;

			thisPos.z = cTex->Size.x * size;
			thisPos.w = cTex->Size.y * size;
			thisPos.z /= 1;
			thisPos.w /= 1;

			vectPushConstants vps{
				vkhelper::calcMatFromRect(thisPos, 0),
				glm::mat4(1.0f)
				};   
			vps.normalMat[3][3] = 1.0;
			vkCmdPushConstants(mSwapchain.frameData[mImg].commandBuffer, pipeline3D.layout, VK_SHADER_STAGE_VERTEX_BIT,
							0, sizeof(vectPushConstants), &vps);

			mModelLoader.drawQuad(mSwapchain.frameData[mImg].commandBuffer, pipeline3D.layout, cTex->TextureID, 1, 0,
			 colour, glm::vec4(0, 0, 1, 1));
		}
		position.x += cTex->Advance * size;
		
	}
}

float Render::MeasureString(Resource::Font* font, std::string text, float size)
{
	float sz = 0;
	for (std::string::const_iterator c = text.begin(); c != text.end(); c++)
	{
		Resource::Character* cTex = font->getChar(*c);
		if (cTex == nullptr)
			continue;
		sz += cTex->Advance * size;
	}
	return sz;
}

void Render::drawBatch()
{
	//std::cout << "batch" << std::endl;
	vectPushConstants vps{
			glm::mat4(1.0f),
			glm::mat4(0.0f)
		};   
	vkCmdPushConstants(mSwapchain.frameData[mImg].commandBuffer, pipeline3D.layout, VK_SHADER_STAGE_VERTEX_BIT,
							0, sizeof(vectPushConstants), &vps);

	if(m3DRender)
		mModelLoader.drawModel(mSwapchain.frameData[mImg].commandBuffer, pipeline3D.layout, currentModel, modelRuns, currentIndex);
	else
		mModelLoader.drawQuad(mSwapchain.frameData[mImg].commandBuffer, pipeline3D.layout, currentTexture.ID, modelRuns, currentIndex, currentColour, currentTexOffset);

	currentIndex += modelRuns;
	modelRuns = 0;

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

	
	viewProjectionData3D.proj = glm::perspective(glm::radians(projectionFov),
			((float)mSwapchain.extent.width / correction) / ((float)mSwapchain.extent.height / correction), 0.1f, 500.0f);
	viewProjectionData3D.proj[1][1] *= -1; //opengl has inversed y axis, so need to correct

}

void Render::setViewMatrixAndFov(glm::mat4 view, float fov)
{
	viewProjectionData3D.view = view;
	projectionFov = fov;
	updateViewProjectionMatrix();
} 
