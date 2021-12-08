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
	initVulkan::CreateDescriptorSets(mBase.device, &mViewprojDS,
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER}, {1}, 
		VK_SHADER_STAGE_VERTEX_BIT);
	initVulkan::CreateDescriptorSets(mBase.device, &mTexturesDS, 
		{VK_DESCRIPTOR_TYPE_SAMPLER, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE}, {1, Resource::MAX_TEXTURES_SUPPORTED}, 
		VK_SHADER_STAGE_FRAGMENT_BIT);
	initVulkan::CreateDescriptorSets(mBase.device, &mLightingDS,
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER}, {1},
		VK_SHADER_STAGE_FRAGMENT_BIT);
	initVulkan::graphicsPipeline(mBase.device, &mPipeline, mSwapchain, mRenderPass, { mViewprojDS.layout, mTexturesDS.layout, mLightingDS.layout });
	prepareDescriptorSet(mViewprojDS, mMemory.viewProj, mSwapchain.frameData.size());
	prepareDescriptorSet(mLightingDS, mMemory.lighting, mSwapchain.frameData.size());
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
	prepareFragmentDescriptorSet();
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

	//copy viewproj data into descriptor set
	std::memcpy(static_cast<char*>(mMemory.viewProj.pointer) + (mImg * mMemory.viewProj.slotSize), &viewProjectionData, mMemory.viewProj.slotSize);
	//copy lighting data into descriptor set
	std::memcpy(static_cast<char*>(mMemory.lighting.pointer) + (mImg * mMemory.lighting.slotSize), &lightingData, mMemory.lighting.slotSize);

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

	//begin render pass
	vkCmdBeginRenderPass(mSwapchain.frameData[mImg].commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	//bind descriptor sets
	vkCmdBindDescriptorSets(mSwapchain.frameData[mImg].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		mPipeline.layout, 0, 1, &mViewprojDS.sets[mImg], 0, nullptr);

	vkCmdBindDescriptorSets(mSwapchain.frameData[mImg].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		mPipeline.layout, 1, 1, &mTexturesDS.sets[mImg], 0, nullptr);

	vkCmdBindDescriptorSets(mSwapchain.frameData[mImg].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		mPipeline.layout, 2, 1, &mLightingDS.sets[mImg], 0, nullptr);

	//bind graphics pipeline
	vkCmdBindPipeline(mSwapchain.frameData[mImg].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.pipeline);
	//bind vertex buffer
	mModelLoader.bindBuffers(mSwapchain.frameData[mImg].commandBuffer);
}

void Render::endDraw()
{
	if (!mBegunDraw)
		throw std::runtime_error("start draw before ending it");
	mBegunDraw = false;
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

void Render::resize()
{
	vkDeviceWaitIdle(mBase.device);
	destroyFrameResources();

	initFrameResources();
	prepareFragmentDescriptorSet();

	vkDeviceWaitIdle(mBase.device);
	updateViewProjectionMatrix();
}

void Render::DrawModel(Resource::Model model, glm::mat4 modelMatrix)
{
	//push constants
	vectPushConstants vps{};
	vps.model = modelMatrix;
	vps.normalMat = glm::transpose(glm::inverse(modelMatrix));
	vkCmdPushConstants(mSwapchain.frameData[mImg].commandBuffer, mPipeline.layout, VK_SHADER_STAGE_VERTEX_BIT,
		0, sizeof(vectPushConstants), &vps);

	mModelLoader.drawModel(mSwapchain.frameData[mImg].commandBuffer, mPipeline.layout, model);
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
			(float)mSwapchain.extent.width / (float)mSwapchain.extent.height, 0.1f, 50.0f);
	viewProjectionData.proj[1][1] *= -1; //opengl has inversed y axis, so need to correct
}

void Render::setViewMatrixAndFov(glm::mat4 view, float fov)
{
	viewProjectionData.view = view;
	projectionFov = fov;
	updateViewProjectionMatrix();
}

void Render::destroyFrameResources()
{
	vkDestroyBuffer(mBase.device, mMemory.viewProj.buffer, nullptr);
	vkFreeMemory(mBase.device, mMemory.viewProj.memory, nullptr);
	vkDestroyBuffer(mBase.device, mMemory.lighting.buffer, nullptr);
	vkFreeMemory(mBase.device, mMemory.lighting.memory, nullptr);
	mViewprojDS.destroySet(mBase.device);
	mTexturesDS.destroySet(mBase.device);
	mLightingDS.destroySet(mBase.device);
	for (size_t i = 0; i < mSwapchain.frameData.size(); i++)
		vkDestroyFramebuffer(mBase.device, mSwapchain.frameData[i].framebuffer, nullptr);
	vkDestroyPipeline(mBase.device, mPipeline.pipeline, nullptr);
	vkDestroyPipelineLayout(mBase.device, mPipeline.layout, nullptr);
	vkDestroyRenderPass(mBase.device, mRenderPass, nullptr);
}


void Render::prepareFragmentDescriptorSet()
{
	vkhelper::prepareDS(mBase.device, mTexturesDS, Resource::MAX_TEXTURES_SUPPORTED);

	std::vector<VkDescriptorImageInfo> texInfos(Resource::MAX_TEXTURES_SUPPORTED);
	for (uint32_t i = 0; i < Resource::MAX_TEXTURES_SUPPORTED; i++)
	{
		texInfos[i].sampler = nullptr;
		texInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		texInfos[i].imageView = mTextureLoader.getImageView(i);
	}

	VkDescriptorImageInfo imgSamplerInfo = {};
	imgSamplerInfo.sampler = mTextureLoader.sampler;

	//sampler
	std::vector<VkWriteDescriptorSet> sampDSWrite(mTexturesDS.sets.size() * 2, { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET });
	int index = 0;
	for (size_t i = 0; i < mTexturesDS.sets.size() * 2; i+=2)
	{
		sampDSWrite[i].dstSet = mTexturesDS.sets[index];
		sampDSWrite[i].pBufferInfo = 0;
		sampDSWrite[i].dstBinding = 0;
		sampDSWrite[i].dstArrayElement = 0;
		sampDSWrite[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		sampDSWrite[i].descriptorCount = 1;
		sampDSWrite[i].pImageInfo = &imgSamplerInfo; //todo sampler

		sampDSWrite[i + 1].dstSet = mTexturesDS.sets[index];
		sampDSWrite[i + 1].pBufferInfo = 0;
		sampDSWrite[i + 1].dstBinding = 1;
		sampDSWrite[i + 1].dstArrayElement = 0;
		sampDSWrite[i + 1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		sampDSWrite[i + 1].descriptorCount = Resource::MAX_TEXTURES_SUPPORTED;
		sampDSWrite[i + 1].pImageInfo = texInfos.data();
		index++;
	}
	vkUpdateDescriptorSets(mBase.device, mTexturesDS.sets.size() * 2, sampDSWrite.data(), 0, nullptr);
}

void Render::prepareDescriptorSet(DS::DescriptorSets &ds, UniformBufferTypes &ubt, size_t setCount)
{
	vkhelper::prepareDS(mBase.device, ds, setCount);

	VkDeviceSize slot = sizeof(DS::viewProjection);
	VkPhysicalDeviceProperties physDevProps;
	vkGetPhysicalDeviceProperties(mBase.physicalDevice, &physDevProps);
	if (slot % physDevProps.limits.minUniformBufferOffsetAlignment != 0)
		slot = slot + physDevProps.limits.minUniformBufferOffsetAlignment
		- (slot % physDevProps.limits.minUniformBufferOffsetAlignment);

	ubt.memSize = slot * ds.sets.size();

	vkhelper::createBufferAndMemory(mBase, ubt.memSize, &ubt.buffer, &ubt.memory, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
		(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));

	vkBindBufferMemory(mBase.device, ubt.buffer, ubt.memory, 0);
	vkMapMemory(mBase.device, ubt.memory, 0, ubt.memSize, 0, &ubt.pointer);

	for (size_t i = 0; i < ds.sets.size(); i++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = ubt.buffer;
		bufferInfo.offset = slot * i;
		bufferInfo.range = slot;

		VkWriteDescriptorSet writeSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		writeSet.dstSet = ds.sets[i];
		writeSet.pBufferInfo = &bufferInfo;
		writeSet.dstBinding = 0;
		writeSet.dstArrayElement = 0;
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeSet.descriptorCount = 1;

		vkUpdateDescriptorSets(mBase.device, 1, &writeSet, 0, nullptr);
	}
	ubt.slotSize = slot;
}
