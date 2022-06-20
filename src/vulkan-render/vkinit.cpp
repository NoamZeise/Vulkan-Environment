#include "vkinit.h"
#include "config.h"
#include "vulkan/vulkan_core.h"
#include <stdexcept>


void initVulkan::Instance(VkInstance* instance)
{
	VkInstanceCreateInfo instanceCreateInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };

	//app info
	VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName = "Vulkan App";
	appInfo.pEngineName = "No Engine";
	appInfo.apiVersion = VK_API_VERSION_1_0;
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	instanceCreateInfo.pApplicationInfo = &appInfo; //give to instance create info

													//extensions
	uint32_t requiredExtensionsCount = 0;
	const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionsCount);
	std::vector<const char*> extensions(requiredExtensions, requiredExtensions + requiredExtensionsCount);
#ifndef NDEBUG
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

	//setup debug features
#ifndef NDEBUG
	if (!_validationLayersSupported())
		throw std::runtime_error("validation layers were requested, but aren't supported");

	instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(OPTIONAL_LAYERS.size());
	instanceCreateInfo.ppEnabledLayerNames = OPTIONAL_LAYERS.data();

	VkValidationFeaturesEXT validationFeatures{ VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
	validationFeatures.enabledValidationFeatureCount = static_cast<uint32_t>(VALIDATION_LAYER_FEATURES.size());
	validationFeatures.pEnabledValidationFeatures = VALIDATION_LAYER_FEATURES.data();
	instanceCreateInfo.pNext = &validationFeatures; //call after create instance

													//setup debug messenger for just the creation of an instance
	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
	_populateDebugMessengerCreateInfo(&debugMessengerInfo);
	validationFeatures.pNext = &debugMessengerInfo; //call after validation feature creation
#else
	instanceCreateInfo.enabledLayerCount = 0;
	instanceCreateInfo.pNext = nullptr;
#endif

	//create instance
	if (vkCreateInstance(&instanceCreateInfo, nullptr, instance) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create instance!");
	}
}

void initVulkan::Device(VkInstance instance, Base* base, VkSurfaceKHR surface)
{
	uint32_t deviceCount;
	if (vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr) != VK_SUCCESS)
		throw std::runtime_error("failed to get device count!");
	if (deviceCount < 1)
		throw std::runtime_error("no gpus detected!");

	std::vector<VkPhysicalDevice> gpus(deviceCount);
	if (vkEnumeratePhysicalDevices(instance, &deviceCount, gpus.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to get device list!");
	//find best device
	bool foundSuitable = false;
	VkPhysicalDeviceProperties bestDeviceProperties;
	VkPhysicalDeviceFeatures bestDeviceFeatures;
	std::vector<VkQueueFamilyProperties> bestQueueFamilies;
	for (size_t i = 0; i < gpus.size(); i++)
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(gpus[i], &deviceProperties);

		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(gpus[i], &deviceFeatures);

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(gpus[i], &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(gpus[i], &queueFamilyCount, queueFamilies.data());



		//std::cout << "push const size : " << deviceProperties.limits.maxPushConstantsSize << std::endl;
		//std::cout << "per stage storage buffer size: " << deviceProperties.limits.maxPerStageDescriptorStorageBuffers << std::endl;
		//std::cout << "all storage buffer size: " << deviceProperties.limits.maxDescriptorSetStorageBuffers << std::endl;

		//supports graphics and present queues?
		if (!foundSuitable || deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) //prioritise discrete gpu
		{
			VkBool32 graphicQueueSupport = VK_FALSE;
			VkBool32 presentQueueSupport = VK_FALSE;
			uint32_t graphicsPresent;
			for (uint32_t j = 0; j < queueFamilies.size(); j++)
			{
				vkGetPhysicalDeviceSurfaceSupportKHR(gpus[i], j, surface, &presentQueueSupport);
				if (queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentQueueSupport)
				{
					graphicQueueSupport = VK_TRUE;
					graphicsPresent = j;
				}
			}
			if (!(graphicQueueSupport && presentQueueSupport))
				continue;

			base->queue.graphicsPresentFamilyIndex = graphicsPresent;

			base->physicalDevice = gpus[i];
			bestDeviceFeatures = deviceFeatures;
			bestDeviceProperties = deviceProperties;
			bestQueueFamilies = queueFamilies;

			foundSuitable = true;
		}
	}
	if (!foundSuitable)
		throw std::runtime_error("Failed to find device which supports graphics and present queues");

	//create logical device

	VkDeviceCreateInfo deviceInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };

	//create queues
	std::set<uint32_t> uniqueQueueFamilies = { base->queue.graphicsPresentFamilyIndex };
	std::vector<VkDeviceQueueCreateInfo> queueInfos(uniqueQueueFamilies.size());
	float queuePriority = 1.0f;
	int familyCount = 0;
	for (uint32_t familyIndex : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queueCreateInfo.queueFamilyIndex = familyIndex;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		queueInfos[familyCount] = queueCreateInfo;
		familyCount++;
	}

	deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
	deviceInfo.pQueueCreateInfos = queueInfos.data();

	//check requested extensions
	uint32_t extensionCount;
	if (vkEnumerateDeviceExtensionProperties(base->physicalDevice, nullptr, &extensionCount, nullptr) != VK_SUCCESS)
		throw std::runtime_error("failed to find device extenions count");
	std::vector<VkExtensionProperties> deviceExtensions(extensionCount);
	if (vkEnumerateDeviceExtensionProperties(base->physicalDevice, nullptr, &extensionCount, deviceExtensions.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to find device extenions");
	for(const auto& extension : REQUESTED_DEVICE_EXTENSIONS)
	{
		bool found = false;
		for (const auto& supported : deviceExtensions)
		{
          //std::cout << supported.extensionName  << std::endl;
			if (std::strcmp(extension, supported.extensionName) == 0)
			{
				found = true;
				break;
			}
		}
		if (!found)
			throw std::runtime_error("device does not support requested extention");
	}
	deviceInfo.enabledExtensionCount = static_cast<uint32_t>(REQUESTED_DEVICE_EXTENSIONS.size());
	deviceInfo.ppEnabledExtensionNames = REQUESTED_DEVICE_EXTENSIONS.data();

    //get device features
    
	//enable optional device features
	VkPhysicalDeviceFeatures deviceFeatures{};
    if(bestDeviceFeatures.samplerAnisotropy)
    {
      deviceFeatures.samplerAnisotropy = VK_TRUE;
      base->features.samplerAnisotropy = true;
    }
	if(settings::SAMPLE_SHADING)
      if(bestDeviceFeatures.sampleRateShading)
      {
		deviceFeatures.sampleRateShading = VK_TRUE;
        base->features.sampleRateShading = true;
      }

	deviceInfo.pEnabledFeatures = &deviceFeatures;

#ifndef  NDEBUG
	deviceInfo.enabledLayerCount = static_cast<uint32_t>(OPTIONAL_LAYERS.size());
	deviceInfo.ppEnabledLayerNames = OPTIONAL_LAYERS.data();
#else
	deviceInfo.enabledLayerCount = 0;
#endif//NDEBUG

	if (vkCreateDevice(base->physicalDevice, &deviceInfo, nullptr, &base->device) != VK_SUCCESS)
		throw std::runtime_error("failed to create logical device");

	//get queue handles for graphics and present
	vkGetDeviceQueue(base->device, base->queue.graphicsPresentFamilyIndex, 0, &base->queue.graphicsPresentQueue);
}

void initVulkan::Swapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, SwapChain* swapchain, GLFWwindow* window, uint32_t graphicsQueueIndex)
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
		_destroySwapchain(swapchain, device, oldSwapChain);
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

void initVulkan::DestroySwapchain(SwapChain* swapchainStruct, const VkDevice& device)
{
	_destroySwapchain(swapchainStruct, device, swapchainStruct->swapChain);
}

void initVulkan::RenderPass(VkDevice device, VkRenderPass* renderPass, SwapChain swapchain, bool presentOnly)
{
	if(presentOnly)
	{
			//create attachments

		//present attachment
	VkAttachmentReference colourAttachmentRef{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentDescription colourAttachment{};
	colourAttachment.format = swapchain.format.format;
	colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;


	std::vector<VkAttachmentDescription> attachments;
	attachments =  {colourAttachment };
	//create subpass
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colourAttachmentRef;

	std::array<VkSubpassDescription, 1> subpasses = { subpass };

	//depenancy to external events
	std::array<VkSubpassDependency, 1> dependencies;
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].srcStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = 0;
	dependencies[0].dstSubpass = 0;
	dependencies[0].dstStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].dstAccessMask =
		    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo createInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	createInfo.pAttachments = attachments.data();
	createInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
	createInfo.pSubpasses = subpasses.data();
	createInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	createInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(device, &createInfo, nullptr, renderPass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass!");
	}
	else
	{
	//create attachments

		//present attachment
	VkAttachmentReference colourAttachmentRef{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentDescription colourAttachment{};
	colourAttachment.format = swapchain.format.format;
	colourAttachment.samples = swapchain.maxMsaaSamples;
	if(settings::MULTISAMPLING)
	{
		colourAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	else
	{
		colourAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
	colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		//depth attachment
	VkAttachmentReference depthBufferRef{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
	VkAttachmentDescription depthAttachment {};
	depthAttachment.format = swapchain.depthBuffer.format;
	depthAttachment.samples = swapchain.maxMsaaSamples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	//resolve attachment
	VkAttachmentReference resolveAttachmentRef{ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentDescription resolveAttachment {};
	resolveAttachment.format = swapchain.format.format;
	resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;


	std::vector<VkAttachmentDescription> attachments;
	if(settings::MULTISAMPLING)
		attachments = { colourAttachment, depthAttachment, resolveAttachment };
	else
		attachments =  {colourAttachment, depthAttachment };
	//create subpass
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colourAttachmentRef;
	if(settings::MULTISAMPLING)
		subpass.pResolveAttachments = &resolveAttachmentRef;
	subpass.pDepthStencilAttachment = &depthBufferRef;

	std::array<VkSubpassDescription, 1> subpasses = { subpass };

	//depenancy to external events
	std::array<VkSubpassDependency, 2> dependencies;
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].srcStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = 0;
	dependencies[0].dstSubpass = 0;
	dependencies[0].dstStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].dstAccessMask =
		    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].srcStageMask =
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].dstStageMask =
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].dstAccessMask =
		    VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo createInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	createInfo.pAttachments = attachments.data();
	createInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
	createInfo.pSubpasses = subpasses.data();
	createInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	createInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(device, &createInfo, nullptr, renderPass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass!");
	}
}

void initVulkan::Framebuffers(VkDevice device, VkRenderPass renderPass, SwapChain* swapchain, bool presentOnly)
{
	if(presentOnly)
	{
		for (size_t i = 0; i < swapchain->frameData.size(); i++)
		{
			std::vector<VkImageView> attachments { swapchain->frameData[i].view };

			VkFramebufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
			createInfo.renderPass = renderPass;
			createInfo.width = swapchain->swapchainExtent.width;
			createInfo.height = swapchain->swapchainExtent.height;
			createInfo.layers = 1;
			createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			createInfo.pAttachments = attachments.data();

			if (vkCreateFramebuffer(device, &createInfo, nullptr, &swapchain->frameData[i].framebuffer) != VK_SUCCESS)
				throw std::runtime_error("failed to create framebuffer");
		}
	}
	else
	{

		std::vector<VkImageView> attachments;
		if(settings::MULTISAMPLING)
			attachments =
			{
				swapchain->multisampling.view,
				swapchain->depthBuffer.view,
		  		swapchain->offscreen.view
			};
		else
			attachments =
			{
				swapchain->offscreen.view,
				swapchain->depthBuffer.view
			};

		VkFramebufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		createInfo.renderPass = renderPass;
		createInfo.width = swapchain->offscreenExtent.width;
		createInfo.height = swapchain->offscreenExtent.height;
		createInfo.layers = 1;
		createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		createInfo.pAttachments = attachments.data();

		if (vkCreateFramebuffer(device, &createInfo, nullptr, &swapchain->offscreenFramebuffer) != VK_SUCCESS)
			throw std::runtime_error("failed to create framebuffer");
	}
}

void initVulkan::GraphicsPipeline(VkDevice device, Pipeline* pipeline, SwapChain swapchain, VkRenderPass renderPass,
	std::vector<DS::DescriptorSet*> descriptorSets,
	std::vector<VkPushConstantRange> pushConstantsRanges,
	std::string vertexShaderPath, std::string fragmentShaderPath, bool useDepthTest, bool presentOnly,
	std::vector<VkVertexInputAttributeDescription> vertexAttribDesc,
	std::vector<VkVertexInputBindingDescription> vertexBindingDesc
)
{
	pipeline->descriptorSets = descriptorSets;

	//load shader modules
	auto vertexShaderModule = _loadShaderModule(device, vertexShaderPath);
	auto fragmentShaderModule = _loadShaderModule(device, fragmentShaderPath);

	//create pipeline layout
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts(descriptorSets.size());

	for (size_t i = 0; i < descriptorSets.size(); i++)
		descriptorSetLayouts[i] = descriptorSets[i]->layout;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantsRanges.size());
	pipelineLayoutInfo.pPushConstantRanges = pushConstantsRanges.data();
	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipeline->layout) != VK_SUCCESS)
		throw std::runtime_error("failed to create pipeline layout");

	//config input assemby
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	//config vertex input
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttribDesc.size());
	vertexInputInfo.pVertexAttributeDescriptions = vertexAttribDesc.data();
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDesc.size());
	vertexInputInfo.pVertexBindingDescriptions = vertexBindingDesc.data();
	if(presentOnly)
	{
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
	}

	//config viewport and scissor
	VkViewport viewport
	{
		0.0f, 0.0f, // x  y
		(float)swapchain.offscreenExtent.width, (float)swapchain.offscreenExtent.height, //width  height
		0.0f, 1.0f // min/max depth
	};
	VkRect2D scissor{ VkOffset2D{0, 0}, swapchain.offscreenExtent };

	if(presentOnly)
	{
		viewport =
		{
			0.0f, 0.0f, // x  y
			(float)swapchain.swapchainExtent.width, (float)swapchain.swapchainExtent.height, //width  height
			0.0f, 1.0f // min/max depth
		};
		scissor = { VkOffset2D{0, 0}, swapchain.swapchainExtent };
	}

	VkPipelineViewportStateCreateInfo viewportInfo{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	viewportInfo.viewportCount = 1;
	viewportInfo.pViewports = &viewport;
	viewportInfo.scissorCount = 1;
	viewportInfo.pScissors = &scissor;

	//config vertex shader
	VkPipelineShaderStageCreateInfo vertexStageInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	vertexStageInfo.module = vertexShaderModule;
	vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexStageInfo.pName = "main";

	//config rasterization
	VkPipelineRasterizationStateCreateInfo rasterizationInfo{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizationInfo.depthClampEnable = VK_FALSE;
	rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	if(presentOnly)
		rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
	rasterizationInfo.lineWidth = 1.0f;
	rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	//fragment shader
	VkPipelineShaderStageCreateInfo fragmentStageInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	fragmentStageInfo.module = fragmentShaderModule;
	fragmentStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentStageInfo.pName = "main";

	//config multisampler
	VkPipelineMultisampleStateCreateInfo multisampleInfo{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampleInfo.rasterizationSamples = swapchain.maxMsaaSamples;
	if(presentOnly)
	{
		multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	}
	else if(settings::SAMPLE_SHADING && settings::MULTISAMPLING)
	{
		multisampleInfo.minSampleShading = 1.0f;
		multisampleInfo.sampleShadingEnable = VK_TRUE;

	}
	//config depthStencil
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	if(useDepthTest)
	{
		depthStencilInfo.depthTestEnable = VK_TRUE;
		depthStencilInfo.depthWriteEnable = VK_TRUE;
	}
	else
	{
		depthStencilInfo.depthTestEnable = VK_FALSE;
		depthStencilInfo.depthWriteEnable = VK_FALSE;
	}

	depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilInfo.depthBoundsTestEnable = VK_FALSE;

	//config colour blend attachment
	VkPipelineColorBlendAttachmentState blendAttachment{};
	blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	blendAttachment.blendEnable = VK_TRUE;
	if(presentOnly)
		blendAttachment.blendEnable = VK_FALSE;
	blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	//config colour blend state
	VkPipelineColorBlendStateCreateInfo blendInfo{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	blendInfo.logicOpEnable = VK_FALSE;
	blendInfo.attachmentCount = 1;
	blendInfo.pAttachments = &blendAttachment;

	//set dynamic states
	//std::array<VkDynamicState, 2> dynamicStates{ };

	VkPipelineDynamicStateCreateInfo dynamicStateInfo{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicStateInfo.dynamicStateCount = 0;
	dynamicStateInfo.pDynamicStates = nullptr;


	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = { vertexStageInfo, fragmentStageInfo };

	//create graphics pipeline
	VkGraphicsPipelineCreateInfo createInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
	createInfo.layout = pipeline->layout;
	createInfo.renderPass = renderPass;
	createInfo.pViewportState = &viewportInfo;
	createInfo.pInputAssemblyState = &inputAssemblyInfo;
	createInfo.pVertexInputState = &vertexInputInfo;
	createInfo.pRasterizationState =  &rasterizationInfo;
	createInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	createInfo.pStages = shaderStages.data();
	createInfo.pMultisampleState = &multisampleInfo;
	createInfo.pDepthStencilState = &depthStencilInfo;
	createInfo.pColorBlendState = &blendInfo;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline->pipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create graphics pipelines!");

	//destory shader modules
	vkDestroyShaderModule(device, vertexShaderModule, nullptr);
	vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
}


void initVulkan::DescriptorSetLayout(VkDevice device, DS::DescriptorSet *ds,
	 std::vector<DS::Binding*> bindings, VkShaderStageFlagBits stageFlags)
{
	//create layout
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings(bindings.size());
	ds->poolSize.resize(bindings.size());
	for(size_t i = 0; i < bindings.size(); i++)
	{
		bindings[i]->binding = static_cast<uint32_t>(i);
		layoutBindings[i].binding = static_cast<uint32_t>(bindings[i]->binding);
		layoutBindings[i].descriptorType = bindings[i]->type;
		layoutBindings[i].descriptorCount = static_cast<uint32_t>(bindings[i]->descriptorCount);
		layoutBindings[i].stageFlags = stageFlags;

		ds->poolSize[i].type = bindings[i]->type;
		ds->poolSize[i].descriptorCount = static_cast<uint32_t>(bindings[i]->descriptorCount);

	}

	VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	layoutInfo.pBindings = layoutBindings.data();
	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &ds->layout) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor sets");
}

void initVulkan::DescriptorPool(VkDevice device, VkDescriptorPool* pool, std::vector<DS::DescriptorSet*> descriptorSets, uint32_t frameCount)
{
  std::vector<VkDescriptorPoolSize> poolSizes;

  for(size_t i = 0; i < descriptorSets.size(); i++)
  {
    for(size_t j = 0; j < descriptorSets[i]->poolSize.size(); j++)
    {
		poolSizes.push_back(descriptorSets[i]->poolSize[j]);
		poolSizes.back().descriptorCount *= frameCount;
    }
  }
  VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = frameCount * static_cast<uint32_t>(descriptorSets.size());
  if(vkCreateDescriptorPool(device, &poolInfo, nullptr, pool) != VK_SUCCESS)
    throw std::runtime_error("Failed to create descriptor pool!");
}

void initVulkan::DescriptorSet(VkDevice device, VkDescriptorPool pool, DS::DescriptorSet *ds, uint32_t frameCount)
{
     std::vector<VkDescriptorSetLayout> layouts(frameCount, ds->layout);
	VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
	allocInfo.pSetLayouts = layouts.data();
	ds->sets.resize(frameCount);
	if (vkAllocateDescriptorSets(device, &allocInfo, ds->sets.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate descriptor sets");
}

void initVulkan::DescriptorPoolAndSet(VkDevice device, VkDescriptorPool* pool, std::vector<DS::DescriptorSet*> descriptorSets, uint32_t frameCount)
{
  DescriptorPool(device, pool, descriptorSets, frameCount);
  for(int i = 0; i < descriptorSets.size(); i++)
  {
    DescriptorSet(device, *pool, descriptorSets[i], frameCount);
  }    
}

void initVulkan::PrepareShaderBufferSets(Base base, std::vector<DS::Binding*> ds, VkBuffer* buffer, VkDeviceMemory* memory)
{
	size_t memSize = _createHostVisibleShaderBufferMemory(base, ds, buffer, memory);

	vkBindBufferMemory(base.device, *buffer, *memory, 0);
	void* pointer;
	vkMapMemory(base.device, *memory, 0, memSize, 0, &pointer);

	for (size_t descI = 0; descI < ds.size(); descI++)
	{
		ds[descI]->pointer = nullptr;

		std::vector<VkWriteDescriptorSet> writes(ds[descI]->setCount, {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET});
		std::vector<VkDescriptorBufferInfo> buffInfos;
		std::vector<VkDescriptorImageInfo> imageInfos;
		if( ds[descI]->type ==  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
			ds[descI]->type == 	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
			ds[descI]->type == 	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
			ds[descI]->type == 	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
        )
		{
			buffInfos.resize(ds[descI]->setCount * ds[descI]->descriptorCount);
			ds[descI]->pointer = pointer;
		}
		if(ds[descI]->type == VK_DESCRIPTOR_TYPE_SAMPLER || ds[descI]->type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
			imageInfos.resize(ds[descI]->setCount * ds[descI]->descriptorCount);

		size_t buffIndex = 0;
		for (size_t i = 0; i < ds[descI]->setCount; i++)
		{
			writes[i].dstSet = ds[descI]->ds->sets[i];
			writes[i].dstBinding = static_cast<uint32_t>(ds[descI]->binding);
			writes[i].dstArrayElement = 0;
			writes[i].descriptorCount = static_cast<uint32_t>(ds[descI]->descriptorCount);
			writes[i].descriptorType = ds[descI]->type;
			switch(ds[descI]->type)
			{
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
					for (size_t j = 0; j < ds[descI]->descriptorCount; j++)
					{
						buffInfos[buffIndex].buffer = *buffer;
						buffInfos[buffIndex].offset = ds[descI]->offset +
							                         (ds[descI]->bufferSize * i) +
							                          (ds[descI]->arraySize * ds[descI]->slotSize * j);
   						buffInfos[buffIndex].range = ds[descI]->slotSize * ds[descI]->arraySize;
						buffIndex++;
					}
					writes[i].pBufferInfo = buffInfos.data() + (i * ds[descI]->descriptorCount);
					break;
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
					for (size_t j = 0; j < ds[descI]->descriptorCount; j++)
					{
						size_t imageIndex = (ds[descI]->descriptorCount * i) + j;
						imageInfos[imageIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						imageInfos[imageIndex].imageView = *(ds[descI]->imageViews + j);
					}
					writes[i].pImageInfo = imageInfos.data() + (i * ds[descI]->descriptorCount);
					break;
				case VK_DESCRIPTOR_TYPE_SAMPLER:
					for (size_t j = 0; j < ds[descI]->descriptorCount; j++)
					{
						size_t imageIndex = (ds[descI]->descriptorCount * i) + j;
						imageInfos[imageIndex].sampler = *(ds[descI]->samplers + j);
					}
					writes[i].pImageInfo = imageInfos.data() + (i * ds[descI]->descriptorCount);
					break;
				default:
					throw std::runtime_error("descriptor type not recognized, in prepare shader buffer sets");
			}
		}
		vkUpdateDescriptorSets(base.device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}
}


//HELPERS

void initVulkan::_fillFrameData(VkDevice device, FrameData* frame, uint32_t graphicsQueueIndex)
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

void initVulkan::_destroySwapchain(SwapChain* swapchainStruct, const VkDevice& device, const VkSwapchainKHR& swapChain)
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

VkShaderModule initVulkan::_loadShaderModule(VkDevice device, std::string file)
{
	VkShaderModule shaderModule;

	std::ifstream in(file, std::ios::binary | std::ios::ate);
	if (!in.is_open())
		throw std::runtime_error("failed to load shader at " + file);

	size_t fileSize = (size_t)in.tellg();
	std::vector<char> shaderCode(fileSize);

	in.seekg(0);
	in.read(shaderCode.data(), fileSize);
	in.close();

	VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.codeSize = shaderCode.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		throw std::runtime_error("failed to create shader module from: " + file);

	return shaderModule;
}

void initVulkan::_createDepthBuffer(VkDevice device, VkPhysicalDevice physicalDevice, SwapChain* swapchain)
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

void initVulkan::_createMultisamplingBuffer(VkDevice device, VkPhysicalDevice physicalDevice, SwapChain* swapchain)
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

void initVulkan::_createAttachmentImageResources(VkDevice device, VkPhysicalDevice physicalDevice,
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

VkFormat initVulkan::_findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features)
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

void initVulkan::_destroyAttachmentImageResources(VkDevice device, AttachmentImage attachment)
{
	vkDestroyImageView(device, attachment.view, nullptr);
	vkDestroyImage(device, attachment.image, nullptr);
	vkFreeMemory(device, attachment.memory, nullptr);
}


size_t initVulkan::_createHostVisibleShaderBufferMemory(Base base, std::vector<DS::Binding*> ds, VkBuffer* buffer, VkDeviceMemory* memory)
{
	VkPhysicalDeviceProperties physDevProps;
	vkGetPhysicalDeviceProperties(base.physicalDevice, &physDevProps);

	size_t memorySize = 0;
	for (size_t i = 0; i < ds.size(); i++)
	{
		if(ds[i]->type != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER &&
		   ds[i]->type != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC &&
		   ds[i]->type != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER &&
		   ds[i]->type != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
		{
			continue;
		}

		VkDeviceSize alignment;
		if(ds[i]->type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
		   ds[i]->type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
			alignment = physDevProps.limits.minStorageBufferOffsetAlignment;
		else
			alignment = physDevProps.limits.minUniformBufferOffsetAlignment;

		ds[i]->slotSize = vkhelper::correctAlignment(ds[i]->dataStructSize, alignment);
		memorySize = vkhelper::correctAlignment(memorySize, alignment);

		ds[i]->offset = memorySize;
		ds[i]->bufferSize = ds[i]->slotSize * ds[i]->descriptorCount * ds[i]->arraySize;
		memorySize += ds[i]->bufferSize * ds[i]->dynamicBufferCount * ds[i]->setCount;
	}

	vkhelper::createBufferAndMemory(base, memorySize, buffer, memory,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	return memorySize;
}


//DEBUG FUNCTIONS
#ifndef NDEBUG
void initVulkan::DebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT* messenger)
{
	//setup debug messenger for all operations
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	_populateDebugMessengerCreateInfo(&createInfo);
	if (_createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, messenger) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

void initVulkan::_populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* createInfo)
{
	//debug messenger settings
	createInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	if(settings::ERROR_ONLY)
		createInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	else
		createInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT //all types
		| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo->pfnUserCallback = _debugUtilsMessengerCallback;
	createInfo->pUserData = nullptr; //optional pointer to user type
}

bool initVulkan::_validationLayersSupported()
{
	//check if validation layer and selected optional layers are supported
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
	for (auto layer : OPTIONAL_LAYERS)
	{
		bool layerSupported = false;
		for (size_t i = 0; i < layerCount; i++)
		{
			if (std::strcmp(layer, availableLayers[i].layerName) == 0)
			{
				layerSupported = true;
				break;
			}
			//std::cout << availableLayers[i].layerName << std::endl;
		}
		if (!layerSupported)
		{
			return false;
		}
	}
	return true;
}

VkResult initVulkan::_createDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	//returns nullptr if function couldnt be loaded
	auto func =
		(PFN_vkCreateDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void initVulkan::DestroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	auto func =
		(PFN_vkDestroyDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}


VKAPI_ATTR VkBool32 VKAPI_CALL initVulkan::_debugUtilsMessengerCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	//write out warnings and errors
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		std::cout << "Warning: " << pCallbackData->messageIdNumber << ":" << pCallbackData->pMessageIdName << ":" << pCallbackData->pMessage << std::endl;
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		std::cerr << "Error: " << pCallbackData->messageIdNumber << ":" << pCallbackData->pMessageIdName << ":" << pCallbackData->pMessage << std::endl;
	}
	return VK_FALSE;
}

#endif
