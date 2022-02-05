#include "render.h"

Render::Render(GLFWwindow* window)
{
	_initRender(window);
	targetResolution = glm::vec2(mSwapchain.extent.width, mSwapchain.extent.height);
}

Render::Render(GLFWwindow* window, glm::vec2 target)
{
	_initRender(window);
	targetResolution = target;
}

void Render::_initRender(GLFWwindow* window)
{
	mWindow = window;
	initVulkan::Instance(&mInstance);
#ifndef NDEBUG
	initVulkan::DebugMessenger(mInstance, &mDebugMessenger);
#endif
	if (glfwCreateWindowSurface(mInstance, mWindow, nullptr, &mSurface) != VK_SUCCESS)
		throw std::runtime_error("failed to create window surface!");
	initVulkan::Device(mInstance, mBase.physicalDevice, &mBase.device, mSurface, &mBase.queue);


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
}

Render::~Render()
{
	vkQueueWaitIdle(mBase.queue.graphicsPresentQueue);
	mTextureLoader.~TextureLoader();
	mModelLoader.~ModelLoader();
	_destroyFrameResources();
	vkDestroyCommandPool(mBase.device, mGeneralCommandPool, nullptr);
	initVulkan::DestroySwapchain(&mSwapchain, mBase.device);
	vkDestroyDevice(mBase.device, nullptr);
	vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
#ifndef NDEBUG
	initVulkan::DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
#endif
	vkDestroyInstance(mInstance, nullptr);
}

void Render::_initFrameResources()
{
	initVulkan::Swapchain(mBase.device, mBase.physicalDevice, mSurface, &mSwapchain, mWindow, mBase.queue.graphicsPresentFamilyIndex);
	initVulkan::RenderPass(mBase.device, &mRenderPass, mSwapchain);
	initVulkan::Framebuffers(mBase.device, &mSwapchain, mRenderPass);

	size_t frameCount = mSwapchain.frameData.size();
	
	//vertex descriptor sets
	mVP3D.setBufferProps(frameCount,			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &mVP3Dds, 1);
	initVulkan::DescriptorSetAndLayout(mBase.device, mVP3Dds, 	  {&mVP3D.binding}, 		  							VK_SHADER_STAGE_VERTEX_BIT, frameCount);

	mVP2D.setBufferProps(frameCount, 			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &mVP2Dds, 1);
	initVulkan::DescriptorSetAndLayout(mBase.device, mVP2Dds, 	  {&mVP2D.binding},		  							VK_SHADER_STAGE_VERTEX_BIT, frameCount);

	mPerInstance.setBufferProps(frameCount,	 	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &mPIds, 1);
	initVulkan::DescriptorSetAndLayout(mBase.device, mPIds,   	  {&mPerInstance.binding}, 							VK_SHADER_STAGE_VERTEX_BIT, frameCount);

	//fragment descriptor sets
	mLighting.setBufferProps(frameCount, 		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &mLds, 1);
	initVulkan::DescriptorSetAndLayout(mBase.device, mLds,	  	  {&mLighting.binding}, 				VK_SHADER_STAGE_FRAGMENT_BIT, frameCount);

	mTextureSampler.setBufferProps(frameCount,	VK_DESCRIPTOR_TYPE_SAMPLER, 	   &mTexturesDS, 1, nullptr, mTextureLoader.getSamplerP());
	mTextureViews.setBufferProps(frameCount,	VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  &mTexturesDS, Resource::MAX_TEXTURES_SUPPORTED, mTextureLoader.getImageViewsP(), nullptr);
	initVulkan::DescriptorSetAndLayout(mBase.device, mTexturesDS, {&mTextureSampler.binding, &mTextureViews.binding},   VK_SHADER_STAGE_FRAGMENT_BIT, frameCount);


	initVulkan::PrepareShaderBufferSets(mBase, {&mVP3D.binding, &mVP2D.binding,  &mPerInstance.binding,  &mLighting.binding, &mTextureSampler.binding, &mTextureViews.binding}, &mShaderBuffer, &mShaderMemory);


	initVulkan::GraphicsPipeline(mBase.device, &mPipeline3D, mSwapchain, mRenderPass, 
			{ &mVP3Dds, &mPIds, &mTexturesDS, &mLds},
			{{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vectPushConstants)},
			{VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(vectPushConstants), sizeof(fragPushConstants)}},
			"shaders/v3D-lighting.spv", "shaders/fblinnphong.spv", true);

	initVulkan::GraphicsPipeline(mBase.device, &mPipeline2D, mSwapchain, mRenderPass, 
			{ &mVP2Dds, &mPIds, &mTexturesDS},
			{{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vectPushConstants)},
			{VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(vectPushConstants), sizeof(fragPushConstants)}},
			"shaders/vflat.spv", "shaders/fflat.spv", false);
	
	_updateViewProjectionMatrix();
	for(size_t i = 0; i < DS::ShaderStructs::MAX_BATCH_SIZE; i++)
	{
		mPerInstance.data[0].model[i] = glm::mat4(1.0f);
		mPerInstance.data[0].normalMat[i] = glm::mat4(1.0f);
	}
}

void Render::_destroyFrameResources()
{
	vkDestroyBuffer(mBase.device, mShaderBuffer, nullptr);
	vkFreeMemory(mBase.device, mShaderMemory, nullptr);

	mVP3Dds.destroySet(mBase.device);
	mVP2Dds.destroySet(mBase.device);
	mPIds.destroySet(mBase.device);
	mLds.destroySet(mBase.device);
	mTexturesDS.destroySet(mBase.device);

	for (size_t i = 0; i < mSwapchain.frameData.size(); i++)
		vkDestroyFramebuffer(mBase.device, mSwapchain.frameData[i].framebuffer, nullptr);
	mPipeline3D.destroy(mBase.device);
	mPipeline2D.destroy(mBase.device);
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
	_initFrameResources();
}

void Render::_resize()
{
	vkDeviceWaitIdle(mBase.device);

	_destroyFrameResources();
	_initFrameResources();

	vkDeviceWaitIdle(mBase.device);
	_updateViewProjectionMatrix();
}

void Render::_startDraw()
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
		_startDraw();
	if(modelRuns > 0)
		_drawBatch();
	m3DRender = true; 
		
	mVP3D.storeData(mImg);
	DS::ShaderStructs::lighting tempLightingData = mLighting.data[0];
	mLighting.data[0].direction = glm::transpose(glm::inverse(mVP3D.data[0].view)) * glm::vec4(0.3f, -0.3f, -0.5f, 0.0f);
	mLighting.storeData(mImg);

	mPipeline3D.begin(mSwapchain.frameData[mImg].commandBuffer, mImg);	
}

void Render::begin2DDraw()
{
	if(!mBegunDraw)
		_startDraw();
	if(modelRuns > 0)
		_drawBatch();
	m3DRender = false;

	float correction;
	float deviceRatio = (float)mSwapchain.extent.width / (float)mSwapchain.extent.height;
	float virtualRatio = targetResolution.x / targetResolution.y;
	float xCorrection = mSwapchain.extent.width / targetResolution.x;
	float yCorrection = mSwapchain.extent.height / targetResolution.y;

	if (virtualRatio < deviceRatio) {
		correction = yCorrection;
	}
	else {
		correction = xCorrection;
	}
	mVP2D.data[0].proj = glm::ortho(0.0f, (float)mSwapchain.extent.width / correction, 0.0f, (float)mSwapchain.extent.height / correction, -1.0f, 1.0f);
	mVP2D.data[0].view = glm::mat4(1.0f);

	mVP2D.storeData(mImg);

	mPipeline2D.begin(mSwapchain.frameData[mImg].commandBuffer, mImg);
}


void Render::_drawBatch()
{
	//std::cout << "batch" << std::endl;
	vectPushConstants vps{
			glm::mat4(1.0f),
			glm::mat4(0.0f)
		};   
	vkCmdPushConstants(mSwapchain.frameData[mImg].commandBuffer, mPipeline3D.layout, VK_SHADER_STAGE_VERTEX_BIT,
							0, sizeof(vectPushConstants), &vps);

	if(m3DRender)
		mModelLoader.drawModel(mSwapchain.frameData[mImg].commandBuffer, mPipeline3D.layout, currentModel, modelRuns, currentIndex);
	else
		mModelLoader.drawQuad(mSwapchain.frameData[mImg].commandBuffer, mPipeline3D.layout, currentTexture.ID, modelRuns, currentIndex, currentColour, currentTexOffset);

	currentIndex += modelRuns;
	modelRuns = 0;

}

void Render::DrawModel(Resource::Model model, glm::mat4 modelMatrix, glm::mat4 normalMat)
{
	if(currentIndex >= DS::ShaderStructs::MAX_BATCH_SIZE)
	{
		std::cout << "single" << std::endl;
		vectPushConstants vps{
			modelMatrix,
			normalMat
		};   
		vps.normalMat[3][3] = 1.0;
		vkCmdPushConstants(mSwapchain.frameData[mImg].commandBuffer, mPipeline3D.layout, VK_SHADER_STAGE_VERTEX_BIT,
							0, sizeof(vectPushConstants), &vps);

		mModelLoader.drawModel(mSwapchain.frameData[mImg].commandBuffer, mPipeline3D.layout, model, 1, 0);
		return;
	}
	
	if(currentModel.ID != model.ID && modelRuns != 0)
		_drawBatch();
	//add model to buffer
	currentModel = model;
	mPerInstance.data[0].model[currentIndex + modelRuns] = modelMatrix;
	mPerInstance.data[0].normalMat[currentIndex + modelRuns] = normalMat;
	modelRuns++;

	if(currentIndex + modelRuns == DS::ShaderStructs::MAX_BATCH_SIZE)
		_drawBatch();
}

void Render::DrawQuad(const Resource::Texture& texture, glm::mat4 modelMatrix, glm::vec4 colour, glm::vec4 texOffset)
{
	if(currentIndex >= DS::ShaderStructs::MAX_BATCH_SIZE)
	{
		std::cout << "single" << std::endl;

		vectPushConstants vps{
			modelMatrix,
			glm::mat4(1.0f)
		};   
		
		vps.normalMat[3][3] = 1.0;
		vkCmdPushConstants(mSwapchain.frameData[mImg].commandBuffer, mPipeline3D.layout, VK_SHADER_STAGE_VERTEX_BIT,
							0, sizeof(vectPushConstants), &vps);

		mModelLoader.drawQuad(mSwapchain.frameData[mImg].commandBuffer, mPipeline3D.layout, texture.ID,
		 	1, 0, colour, texOffset);
		return;
	}

	if( modelRuns != 0 && 
		(currentTexture.ID != currentTexture.ID ||
		 currentTexOffset != texOffset ||
		 currentColour    != colour))
	{
		_drawBatch();
	}

	//add model to buffer
	currentTexture = texture;
	currentColour = colour;
	currentTexOffset = texOffset;
	mPerInstance.data[0].model[currentIndex + modelRuns] = modelMatrix;
	modelRuns++;

	if(currentIndex + modelRuns == DS::ShaderStructs::MAX_BATCH_SIZE)
		_drawBatch();
}

void Render::DrawQuad(const Resource::Texture& texture, glm::mat4 modelMatrix, glm::vec4 colour)
{
	DrawQuad(texture, modelMatrix, colour, glm::vec4(0, 0, 1, 1));
}

void Render::DrawQuad(const Resource::Texture& texture, glm::mat4 modelMatrix)
{
	DrawQuad(texture, modelMatrix, glm::vec4(1), glm::vec4(0, 0, 1, 1));
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
				glmhelper::calcMatFromRect(thisPos, 0),
				glm::mat4(1.0f)
				};   
			vps.normalMat[3][3] = 1.0;
			vkCmdPushConstants(mSwapchain.frameData[mImg].commandBuffer, mPipeline3D.layout, VK_SHADER_STAGE_VERTEX_BIT,
							0, sizeof(vectPushConstants), &vps);

			mModelLoader.drawQuad(mSwapchain.frameData[mImg].commandBuffer, mPipeline3D.layout, cTex->TextureID, 1, 0,
			 colour, glm::vec4(0, 0, 1, 1));
		}
		position.x += cTex->Advance * size;
		
	}
}

void Render::endDraw(std::atomic<bool>& submit)
{
	if (!mBegunDraw)
		throw std::runtime_error("start draw before ending it");
	mBegunDraw = false;

	if(modelRuns != 0 && currentIndex < DS::ShaderStructs::MAX_BATCH_SIZE)
		_drawBatch();

	mPerInstance.storeData(mImg);
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

	if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR || mFramebufferResized)
	{
		mFramebufferResized = false;
		_resize();
	}
	else if (result != VK_SUCCESS)
		throw std::runtime_error("failed to present swapchain image to queue");

	mSwapchain.imageAquireSem.push_back(mImgAquireSem);

	submit = true;
}

void Render::_updateViewProjectionMatrix()
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

	
	mVP3D.data[0].proj = glm::perspective(glm::radians(mProjectionFov),
			((float)mSwapchain.extent.width / correction) / ((float)mSwapchain.extent.height / correction), 0.1f, 500.0f);
	mVP3D.data[0].proj[1][1] *= -1; //opengl has inversed y axis, so need to correct

}

void Render::setViewMatrixAndFov(glm::mat4 view, float fov)
{
	mVP3D.data[0].view = view;
	mProjectionFov = fov;
	_updateViewProjectionMatrix();
} 

void Render::framebufferResize()
{
	mFramebufferResized = true;
}