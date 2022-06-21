#include "swapchain.h"

namespace part
{
namespace destroy
{

void _swapchain(SwapChain* swapchain, const VkDevice& device, const VkSwapchainKHR& oldSwapChain);

}
namespace create
{

void _createDepthBuffer(VkDevice device, VkPhysicalDevice physicalDevice, SwapChain* swapchain);
void _createMultisamplingBuffer(VkDevice device, VkPhysicalDevice physicalDevice, SwapChain* swapchain);
void _createAttachmentImageResources(VkDevice device, VkPhysicalDevice physicalDevice, AttachmentImage* attachIm, SwapChain& swapchain, VkImageUsageFlags usage, VkImageAspectFlags imgAspect, VkSampleCountFlagBits samples);
VkFormat _findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features);
void _fillFrameData(VkDevice device, FrameData* frame, uint32_t graphicsQueueIndex);

void Swapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, SwapChain* swapchain, GLFWwindow* window, uint32_t graphicsQueueIndex)
{
	//get surface formats
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> formats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());
	//chose a format
	if (formatCount == 0)
		throw std::runtime_error("no formats available");
	else if (formatCount == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
	{
		swapchain->format = formats[0];
		if(settings::SRGB)
			swapchain->format.format = VK_FORMAT_R8G8B8A8_SRGB;
		else
			swapchain->format.format = VK_FORMAT_R8G8B8A8_UNORM;
	}
	else
	{
		swapchain->format.format = VK_FORMAT_UNDEFINED;
		for (auto& fmt : formats)
		{
			switch (fmt.format)
			{
			case VK_FORMAT_R8G8B8A8_SRGB:
				if (settings::SRGB)
					swapchain->format = fmt;
				break;
			case VK_FORMAT_R8G8B8A8_UNORM:
			case VK_FORMAT_B8G8R8A8_UNORM:
			case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
				if (swapchain->format.format != VK_FORMAT_R8G8B8A8_SRGB)
					swapchain->format = fmt;
				break;
			}
		}
		if (swapchain->format.format == VK_FORMAT_UNDEFINED)
		{
			swapchain->format = formats[0];
		}
	}

	//get surface capabilities
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities) != VK_SUCCESS)
		throw std::runtime_error("failed to get physical device surface capabilities!");
	//get image count
	uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
	if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
		imageCount = surfaceCapabilities.maxImageCount;

	//set extent
	swapchain->swapchainExtent = { 0, 0 };
	if (surfaceCapabilities.currentExtent.width != UINT32_MAX)	//cant be modified
	{
		swapchain->swapchainExtent = surfaceCapabilities.currentExtent;
	}
	else
	{
	    int swidth, sheight;
		glfwGetFramebufferSize(window, &swidth, &sheight);
		uint32_t width = static_cast<uint32_t>(swidth);
		uint32_t height = static_cast<uint32_t>(sheight);
		swapchain->swapchainExtent = {
			width,
			height
	    };
		//clamp width
		if (width > surfaceCapabilities.maxImageExtent.width)
			swapchain->swapchainExtent.width = surfaceCapabilities.maxImageExtent.width;
		else if (width < surfaceCapabilities.minImageExtent.width)
			swapchain->swapchainExtent.width = surfaceCapabilities.minImageExtent.width;
		//clamp height
		if (height > surfaceCapabilities.maxImageExtent.height)
			swapchain->swapchainExtent.width = surfaceCapabilities.maxImageExtent.height;
		else if (height < surfaceCapabilities.minImageExtent.height)
			swapchain->swapchainExtent.width = surfaceCapabilities.minImageExtent.height;
	}

	//set offscreen extent

	if(settings::USE_TARGET_RESOLUTION)
		swapchain->offscreenExtent = { settings::TARGET_WIDTH, settings::TARGET_HEIGHT };
	else
		swapchain->offscreenExtent = swapchain->swapchainExtent;

	//choose present mode
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	uint32_t presentModeCount;
	if(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr) != VK_SUCCESS)
		throw std::runtime_error("failed to get physical device surface present mode count!");
	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	if(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to get physical device surface present modes!");
	bool modeChosen = false;

	if(!settings::VSYNC)
	{
		for (const auto& mode : presentModes)
		{
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR) //for low latency
			{
				presentMode = mode;
				modeChosen = true;
			}
			else if (!modeChosen && mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
			{
				presentMode = mode;//for low stuttering
				modeChosen = true;
			}
		}
	}

	//find a supporte transform
	VkSurfaceTransformFlagBitsKHR preTransform;
	if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		preTransform = surfaceCapabilities.currentTransform;
	}

	// Find a supported composite type.
	VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
	{
		compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	}
	else if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
	{
		compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	}
	else if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
	{
		compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
	}
	else if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
	{
		compositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
	}

	VkSwapchainKHR oldSwapChain = swapchain->swapChain;

	//create swapchain
	VkSwapchainCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
	createInfo.flags = 0;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = swapchain->format.format;
	createInfo.imageColorSpace = swapchain->format.colorSpace;
	createInfo.imageExtent = { swapchain->swapchainExtent.width, swapchain->swapchainExtent.height };
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if(surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
	{
		createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}
	if(surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
	{
		createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = oldSwapChain;
	createInfo.compositeAlpha = compositeAlpha;
	createInfo.preTransform = preTransform;
	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain->swapChain) != VK_SUCCESS)
		throw std::runtime_error("failed to create swapchain!");

	if (oldSwapChain != VK_NULL_HANDLE)
	{
		destroy::_swapchain(swapchain, device, oldSwapChain);
	}

	//get swapchain images
	if(vkGetSwapchainImagesKHR(device, swapchain->swapChain, &imageCount, nullptr) != VK_SUCCESS)
		throw std::runtime_error("failed to get swap chain image count!");
	std::vector<VkImage> scImages(imageCount);
	if (vkGetSwapchainImagesKHR(device, swapchain->swapChain, &imageCount, scImages.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to get swap chain images!");
	//create swapchain image views
	swapchain->frameData.resize(imageCount);
	for (size_t i = 0; i < imageCount; i++)
	{
		swapchain->frameData[i].image = scImages[i];

		VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		viewInfo.image = swapchain->frameData[i].image;
		viewInfo.format = swapchain->format.format;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;

		if (vkCreateImageView(device, &viewInfo, nullptr, &swapchain->frameData[i].view) != VK_SUCCESS)
			throw std::runtime_error("failed to create image view");
	}
	//init frame data
	for (size_t i = 0; i < imageCount; i++)
	{
		_fillFrameData(device, &swapchain->frameData[i], graphicsQueueIndex);
	}

	//create attachment resources
	if(settings::MULTISAMPLING)
		_createMultisamplingBuffer(device, physicalDevice, swapchain); //this first as sets max msaa used by rest of attachments
	else
		swapchain->maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;
	_createDepthBuffer(device, physicalDevice, swapchain);
	swapchain->offscreen.format = swapchain->format.format;
	_createAttachmentImageResources(device, physicalDevice, &swapchain->offscreen, *swapchain,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_SAMPLE_COUNT_1_BIT);
}

void _createDepthBuffer(VkDevice device, VkPhysicalDevice physicalDevice, SwapChain* swapchain)
{
	//get a supported format for depth buffer
	swapchain->depthBuffer.format = _findSupportedFormat( physicalDevice,
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );

	_createAttachmentImageResources(device, physicalDevice, &swapchain->depthBuffer, *swapchain,
									VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, swapchain->maxMsaaSamples);
}

void _createMultisamplingBuffer(VkDevice device, VkPhysicalDevice physicalDevice, SwapChain* swapchain)
{
	//get max msaa samples supported by physical device
	swapchain->maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;
	VkPhysicalDeviceProperties props;
 	vkGetPhysicalDeviceProperties(physicalDevice, &props);
	VkSampleCountFlags samplesSupported = props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;
	if     (samplesSupported & VK_SAMPLE_COUNT_64_BIT) swapchain->maxMsaaSamples = VK_SAMPLE_COUNT_64_BIT;
	else if(samplesSupported & VK_SAMPLE_COUNT_32_BIT) swapchain->maxMsaaSamples = VK_SAMPLE_COUNT_32_BIT;
	else if(samplesSupported & VK_SAMPLE_COUNT_16_BIT) swapchain->maxMsaaSamples = VK_SAMPLE_COUNT_16_BIT;
	else if(samplesSupported & VK_SAMPLE_COUNT_8_BIT) swapchain->maxMsaaSamples = VK_SAMPLE_COUNT_8_BIT;
	else if(samplesSupported & VK_SAMPLE_COUNT_4_BIT) swapchain->maxMsaaSamples = VK_SAMPLE_COUNT_4_BIT;
	else if(samplesSupported & VK_SAMPLE_COUNT_2_BIT) swapchain->maxMsaaSamples = VK_SAMPLE_COUNT_2_BIT;

	swapchain->multisampling.format = swapchain->format.format;

	_createAttachmentImageResources(device, physicalDevice, &swapchain->multisampling, *swapchain,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT, swapchain->maxMsaaSamples);
}

void _createAttachmentImageResources(VkDevice device, VkPhysicalDevice physicalDevice,
									AttachmentImage* attachIm, SwapChain& swapchain,
									 VkImageUsageFlags usage, VkImageAspectFlags imgAspect, VkSampleCountFlagBits samples)
{
//create attach image
	VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = swapchain.offscreenExtent.width;
	imageInfo.extent.height = swapchain.offscreenExtent.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = attachIm->format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = samples;

	if (vkCreateImage(device, &imageInfo, nullptr, &attachIm->image) != VK_SUCCESS)
		throw std::runtime_error("failed to create attachment image");

	//assign memory for attach image
	VkMemoryRequirements memreq;
	vkGetImageMemoryRequirements(device, attachIm->image, &memreq);

	vkhelper::createMemory(device, physicalDevice, memreq.size, &attachIm->memory,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memreq.memoryTypeBits);

	vkBindImageMemory(device, attachIm->image, attachIm->memory, 0);

	//create attach image view

	VkImageViewCreateInfo viewInfo { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	viewInfo.image = attachIm->image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = attachIm->format;
	viewInfo.subresourceRange.aspectMask = imgAspect;
	viewInfo.subresourceRange.layerCount = 1;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.baseMipLevel = 0;

	if(vkCreateImageView(device, &viewInfo, nullptr, &attachIm->view) != VK_SUCCESS)
		throw std::runtime_error("Failed to create image view for attachment!");
}

VkFormat _findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (const auto& format : formats)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			return format;
		else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			return format;
	}
	throw std::runtime_error("None of the formats supplied were supported!");
}

void _fillFrameData(VkDevice device, FrameData* frame, uint32_t graphicsQueueIndex)
{
	//create command pool
	VkCommandPoolCreateInfo commandPoolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	commandPoolInfo.queueFamilyIndex = graphicsQueueIndex;
	//commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	if (vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frame->commandPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command buffer");

	//create command buffer
	VkCommandBufferAllocateInfo commandBufferInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	commandBufferInfo.commandPool = frame->commandPool;
	commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferInfo.commandBufferCount = 1;
	if (vkAllocateCommandBuffers(device, &commandBufferInfo, &frame->commandBuffer))
		throw std::runtime_error("failed to allocate command buffer");

	//create semaphores
	VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frame->presentReadySem) != VK_SUCCESS)
		throw std::runtime_error("failed to create present ready semaphore");

	//create fence
	VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	if (vkCreateFence(device, &fenceInfo, nullptr, &frame->frameFinishedFen) != VK_SUCCESS)
		throw std::runtime_error("failed to create frame finished fence");
}

}
namespace destroy
{

void _destroyAttachmentImageResources(VkDevice device, AttachmentImage attachment);

void Swapchain(VkDevice device, SwapChain* swapchain)
{
	_swapchain(swapchain, device, swapchain->swapChain);
}

void _swapchain(SwapChain* swapchainStruct, const VkDevice& device, const VkSwapchainKHR& swapChain)
{
	_destroyAttachmentImageResources(device, swapchainStruct->offscreen);
	_destroyAttachmentImageResources(device, swapchainStruct->depthBuffer);
	if(settings::MULTISAMPLING)
		_destroyAttachmentImageResources(device, swapchainStruct->multisampling);

	for (size_t i = 0; i < swapchainStruct->frameData.size(); i++)
	{
		vkDestroyImageView(device, swapchainStruct->frameData[i].view, nullptr);
		vkFreeCommandBuffers(device, swapchainStruct->frameData[i].commandPool, 1, &swapchainStruct->frameData[i].commandBuffer);
		vkDestroyCommandPool(device, swapchainStruct->frameData[i].commandPool, nullptr);
		vkDestroySemaphore(device, swapchainStruct->frameData[i].presentReadySem, nullptr);
		vkDestroyFence(device, swapchainStruct->frameData[i].frameFinishedFen, nullptr);
	}
	for (size_t i = 0; i < swapchainStruct->imageAquireSem.size(); i++)
	{
		vkDestroySemaphore(device, swapchainStruct->imageAquireSem[i], nullptr);
	}
	swapchainStruct->imageAquireSem.clear();
	swapchainStruct->frameData.clear();
	vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void _destroyAttachmentImageResources(VkDevice device, AttachmentImage attachment)
{
	vkDestroyImageView(device, attachment.view, nullptr);
	vkDestroyImage(device, attachment.image, nullptr);
	vkFreeMemory(device, attachment.memory, nullptr);
}

}
}
