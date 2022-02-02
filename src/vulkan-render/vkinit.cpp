#include "vkinit.h"


void initVulkan::instance(VkInstance* instance)
{
	VkInstanceCreateInfo instanceCreateInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };

	//app info
	VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName = "Vulkan App";
	appInfo.pEngineName = "No Engine";
	appInfo.apiVersion = VK_API_VERSION_1_2;
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	instanceCreateInfo.pApplicationInfo = &appInfo; //give to instance create info

													//extensions
	uint32_t requiredExtensionsCount = 0;
	const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionsCount);
	std::vector<const char*> extensions(requiredExtensions, requiredExtensions + requiredExtensionsCount);
#ifndef NDEBUG
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
	instanceCreateInfo.enabledExtensionCount = extensions.size();
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

	//setup debug features
#ifndef NDEBUG
	if (!validationLayersSupported())
		throw std::runtime_error("validation layers were requested, but aren't supported");

	instanceCreateInfo.enabledLayerCount = OPTIONAL_LAYERS.size();
	instanceCreateInfo.ppEnabledLayerNames = OPTIONAL_LAYERS.data();

	VkValidationFeaturesEXT validationFeatures{ VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
	validationFeatures.enabledValidationFeatureCount = VALIDATION_LAYER_FEATURES.size();
	validationFeatures.pEnabledValidationFeatures = VALIDATION_LAYER_FEATURES.data();
	instanceCreateInfo.pNext = &validationFeatures; //call after create instance

													//setup debug messenger for just the creation of an instance
	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
	populateDebugMessengerCreateInfo(&debugMessengerInfo);
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

void initVulkan::device(VkInstance instance, VkPhysicalDevice& physicalDevice, VkDevice* logicalDevice, VkSurfaceKHR surface, QueueFamilies* families)
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
		//supports graphics and present queues?
		if (!foundSuitable || deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) //prioritise discrete gpu
		{
			VkBool32 graphicQueueSupport = VK_FALSE;
			VkBool32 presentQueueSupport = VK_FALSE;
			uint32_t graphicsPresent;
			for (size_t j = 0; j < queueFamilies.size(); j++)
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

			families->graphicsPresentFamilyIndex = graphicsPresent;

			physicalDevice = gpus[i];
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
	std::set<uint32_t> uniqueQueueFamilies = { families->graphicsPresentFamilyIndex };
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

	deviceInfo.queueCreateInfoCount = queueInfos.size();
	deviceInfo.pQueueCreateInfos = queueInfos.data();

	//check requested extensions
	uint32_t extensionCount;
	if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr) != VK_SUCCESS)
		throw std::runtime_error("failed to find device extenions count");
	std::vector<VkExtensionProperties> deviceExtensions(extensionCount);
	if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, deviceExtensions.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to find device extenions");
	for(const auto& extension : REQUESTED_DEVICE_EXTENSIONS)
	{
		bool found = false;
		for (const auto& supported : deviceExtensions)
		{
			if (std::strcmp(extension, supported.extensionName) == 0)
			{
				found = true;
				break;
			}
		}
		if (!found)
			throw std::runtime_error("device does not support requested extention");
	}
	deviceInfo.enabledExtensionCount = REQUESTED_DEVICE_EXTENSIONS.size();
	deviceInfo.ppEnabledExtensionNames = REQUESTED_DEVICE_EXTENSIONS.data();

	//enable optional device features
	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	if(settings::SAMPLE_SHADING)
		deviceFeatures.sampleRateShading = VK_TRUE;

	deviceInfo.pEnabledFeatures = &deviceFeatures;

#ifndef  NDEBUG
	deviceInfo.enabledLayerCount = OPTIONAL_LAYERS.size();
	deviceInfo.ppEnabledLayerNames = OPTIONAL_LAYERS.data();
#else
	deviceInfo.enabledLayerCount = 0;
#endif//NDEBUG

	if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, logicalDevice) != VK_SUCCESS)
		throw std::runtime_error("failed to create logical device");

	//get queue handles for graphics and present
	vkGetDeviceQueue(*logicalDevice, families->graphicsPresentFamilyIndex, 0, &families->graphicsPresentQueue);
}

void initVulkan::swapChain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, SwapChain* swapchain, GLFWwindow* window, uint32_t graphicsQueueIndex)
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
	uint32_t imageCount = surfaceCapabilities.minImageCount + 1;;
	if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
		imageCount = surfaceCapabilities.maxImageCount;

	//set extent
	swapchain->extent = { 0, 0 };
	if (surfaceCapabilities.currentExtent.width != UINT32_MAX)	//cant be modified
	{
		swapchain->extent = surfaceCapabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		swapchain->extent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height) };
		//clamp width
		if (width > surfaceCapabilities.maxImageExtent.width)
			swapchain->extent.width = surfaceCapabilities.maxImageExtent.width;
		else if (width < surfaceCapabilities.minImageExtent.width)
			swapchain->extent.width = surfaceCapabilities.minImageExtent.width;
		//clamp height
		if (height > surfaceCapabilities.maxImageExtent.height)
			swapchain->extent.width = surfaceCapabilities.maxImageExtent.height;
		else if (height < surfaceCapabilities.minImageExtent.height)
			swapchain->extent.width = surfaceCapabilities.minImageExtent.height;
	}

	//choose present mode
	VkPresentModeKHR presentMode;
	uint32_t presentModeCount;
	if(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr) != VK_SUCCESS)
		throw std::runtime_error("failed to get physical device surface present mode count!");
	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	if(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to get physical device surface present modes!");
	bool modeChosen = false;
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
	if (!modeChosen || settings::VSYNC)
		presentMode = VK_PRESENT_MODE_FIFO_KHR; //guarenteed

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
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.presentMode = presentMode;
	createInfo.imageFormat = swapchain->format.format;
	createInfo.imageColorSpace = swapchain->format.colorSpace;
	createInfo.imageExtent = swapchain->extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = oldSwapChain;
	createInfo.compositeAlpha = compositeAlpha;
	createInfo.preTransform = preTransform;
	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain->swapChain) != VK_SUCCESS)
		throw std::runtime_error("failed to create swapchain!");

	if (oldSwapChain != VK_NULL_HANDLE)
	{
		destroySwapchain(swapchain, device, oldSwapChain);
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
		viewInfo.subresourceRange.levelCount = 1;
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
		fillFrameData(device, &swapchain->frameData[i], graphicsQueueIndex);
	}

	//create attachment resources
	if(settings::MULTISAMPLING)
		createMultisamplingBuffer(device, physicalDevice, swapchain); //this first as sets max msaa used by rest of attachments
	else
		swapchain->maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;
	createDepthBuffer(device, physicalDevice, swapchain);
}

void initVulkan::renderPass(VkDevice device, VkRenderPass* renderPass, SwapChain swapchain)
{
	//create attachments

	//present attachment
	VkAttachmentReference colourAttachmentRef{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentDescription colourAttachment{};
	colourAttachment.format = swapchain.format.format;
	colourAttachment.samples = swapchain.maxMsaaSamples;
	colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	if(settings::MULTISAMPLING)
		colourAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	else
		colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
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
	resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


	std::vector<VkAttachmentDescription> attachments;
	if(settings::MULTISAMPLING)
		attachments = { colourAttachment, depthAttachment, resolveAttachment }; 
	else
		attachments =  { colourAttachment, depthAttachment }; 
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
	VkSubpassDependency externalDependancy{};
	externalDependancy.srcSubpass = VK_SUBPASS_EXTERNAL;
	externalDependancy.srcStageMask = 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | 
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

	externalDependancy.srcAccessMask = 0;
	externalDependancy.dstSubpass = 0;
	externalDependancy.dstStageMask = 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	externalDependancy.dstAccessMask = 
		    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkSubpassDependency, 1> dependancies = { externalDependancy };

	VkRenderPassCreateInfo createInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	createInfo.attachmentCount = attachments.size();
	createInfo.pAttachments = attachments.data();
	createInfo.subpassCount = subpasses.size();
	createInfo.pSubpasses = subpasses.data();
	createInfo.dependencyCount = dependancies.size();
	createInfo.pDependencies = dependancies.data();

	if (vkCreateRenderPass(device, &createInfo, nullptr, renderPass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass!");
}

void initVulkan::framebuffers(VkDevice device, SwapChain* swapchain, VkRenderPass renderPass)
{

	for (size_t i = 0; i < swapchain->frameData.size(); i++)
	{
		std::vector<VkImageView> attachments;
		if(settings::MULTISAMPLING)
			attachments = 
			{ 
		  		swapchain->multisampling.view,
		  		swapchain->depthBuffer.view,
		  		swapchain->frameData[i].view };
		else
			attachments = 
			{ 
		  		swapchain->frameData[i].view,
				swapchain->depthBuffer.view };

		VkFramebufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		createInfo.renderPass = renderPass;
		createInfo.width = swapchain->extent.width;
		createInfo.height = swapchain->extent.height;
		createInfo.layers = 1;
		createInfo.attachmentCount = attachments.size();
		createInfo.pAttachments = attachments.data();

		if (vkCreateFramebuffer(device, &createInfo, nullptr, &swapchain->frameData[i].framebuffer) != VK_SUCCESS)
			throw std::runtime_error("failed to create framebuffer");
	}
}

void initVulkan::graphicsPipeline(VkDevice device, Pipeline* pipeline, SwapChain swapchain, VkRenderPass renderPass,
	std::vector<DS::DescriptorSet*> descriptorSets, 
	std::vector<VkPushConstantRange> pushConstantsRanges,
	std::string vertexShaderPath, std::string fragmentShaderPath)
{
	pipeline->descriptorSets = descriptorSets;

	//load shader modules
	auto vertexShaderModule = loadShaderModule(device, vertexShaderPath);
	auto fragmentShaderModule = loadShaderModule(device, fragmentShaderPath);

	//create pipeline layout
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts(descriptorSets.size());
	
	for (size_t i = 0; i < descriptorSets.size(); i++)
		descriptorSetLayouts[i] = descriptorSets[i]->layout;
	
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = pushConstantsRanges.size();
	pipelineLayoutInfo.pPushConstantRanges = pushConstantsRanges.data();
	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipeline->layout) != VK_SUCCESS)
		throw std::runtime_error("failed to create pipeline layout");

	//config input assemby
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	//config vertex input
	auto attribDescriptions = Vertex::attributeDescriptions();
	auto bindingDescriptions = Vertex::bindingDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputInfo.vertexAttributeDescriptionCount = attribDescriptions.size();
	vertexInputInfo.pVertexAttributeDescriptions = attribDescriptions.data();
	vertexInputInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

	//config viewport and scissor
	VkViewport viewport
	{
		0.0f, 0.0f, // x  y
		(float)swapchain.extent.width, (float)swapchain.extent.height, //width  height
		0.0f, 1.0f // min/max depth
	};
	VkRect2D scissor{ VkOffset2D{0, 0}, swapchain.extent };

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
	if(settings::SAMPLE_SHADING && settings::MULTISAMPLING)
	{	
		multisampleInfo.minSampleShading = 1.0f;
		multisampleInfo.sampleShadingEnable = VK_TRUE;
		
	}
	//config depthStencil
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthStencilInfo.depthTestEnable = VK_TRUE;
	depthStencilInfo.depthWriteEnable = VK_TRUE;
	depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilInfo.depthBoundsTestEnable = VK_FALSE;

	//config colour blend attachment
	VkPipelineColorBlendAttachmentState blendAttachment{};
	blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	blendAttachment.blendEnable = VK_FALSE;
	//config colour blend state
	VkPipelineColorBlendStateCreateInfo blendInfo{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	blendInfo.attachmentCount = 1;
	blendInfo.pAttachments = &blendAttachment;

	//set dynamic states
	std::array<VkDynamicState, 2> dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicStateInfo{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicStateInfo.dynamicStateCount = dynamicStates.size();
	dynamicStateInfo.pDynamicStates = dynamicStates.data();


	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = { vertexStageInfo, fragmentStageInfo };

	//create graphics pipeline
	VkGraphicsPipelineCreateInfo createInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
	createInfo.layout = pipeline->layout;
	createInfo.renderPass = renderPass;
	createInfo.pViewportState = &viewportInfo;
	createInfo.pInputAssemblyState = &inputAssemblyInfo;
	createInfo.pVertexInputState = &vertexInputInfo;
	createInfo.pRasterizationState =  &rasterizationInfo;
	createInfo.stageCount = shaderStages.size();
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



void initVulkan::CreateDescriptorSetLayout(VkDevice device, DS::DescriptorSet* descriptorSets,
	 std::vector<VkDescriptorType> descriptorTypes, std::vector<uint32_t> descriptorCount, VkShaderStageFlagBits stageFlags)
{
	//create layout
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings(descriptorTypes.size());
	descriptorSets->poolSize.resize(descriptorTypes.size());
	for(size_t i = 0; i < descriptorTypes.size(); i++)
	{
		layoutBindings[i].binding = i;
		layoutBindings[i].descriptorType = descriptorTypes[i];
		layoutBindings[i].descriptorCount = descriptorCount[i];
		layoutBindings[i].stageFlags = stageFlags;

		descriptorSets->poolSize[i].type = descriptorTypes[i];
		descriptorSets->poolSize[i].descriptorCount = descriptorCount[i];
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layoutInfo.bindingCount = layoutBindings.size();
	layoutInfo.pBindings = layoutBindings.data();
	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSets->layout) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor sets");
}

//HELPERS

void initVulkan::fillFrameData(VkDevice device, FrameData* frame, uint32_t graphicsQueueIndex)
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

void initVulkan::destroySwapchain(SwapChain* swapchainStruct, const VkDevice& device, const VkSwapchainKHR& swapChain)
{
	destroyAttachmentImageResources(device, swapchainStruct->depthBuffer);
	if(settings::MULTISAMPLING)
		destroyAttachmentImageResources(device, swapchainStruct->multisampling);

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

void initVulkan::destroySwapchain(SwapChain* swapchainStruct, const VkDevice& device)
{
	destroySwapchain(swapchainStruct, device, swapchainStruct->swapChain);
}

VkShaderModule initVulkan::loadShaderModule(VkDevice device, std::string file)
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

void initVulkan::createDepthBuffer(VkDevice device, VkPhysicalDevice physicalDevice, SwapChain* swapchain)
{
	//get a supported format for depth buffer
	swapchain->depthBuffer.format = findSupportedFormat( physicalDevice,
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );

	createAttachmentImageResources(device, physicalDevice, &swapchain->depthBuffer, *swapchain, 
									VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void initVulkan::createMultisamplingBuffer(VkDevice device, VkPhysicalDevice physicalDevice, SwapChain* swapchain)
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
	
	createAttachmentImageResources(device, physicalDevice, &swapchain->multisampling, *swapchain,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
}

void initVulkan::createAttachmentImageResources(VkDevice device, VkPhysicalDevice physicalDevice, 
									AttachmentImage* attachIm, SwapChain& swapchain,
									 VkImageUsageFlags usage, VkImageAspectFlags imgAspect)
{	
//create attach image
	VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = swapchain.extent.width;
	imageInfo.extent.height = swapchain.extent.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = attachIm->format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = swapchain.maxMsaaSamples;

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

VkFormat initVulkan::findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features)
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

void initVulkan::destroyAttachmentImageResources(VkDevice device, AttachmentImage attachment)
{
	vkDestroyImageView(device, attachment.view, nullptr);
	vkDestroyImage(device, attachment.image, nullptr);
	vkFreeMemory(device, attachment.memory, nullptr);
}
//DEBUG FUNCTIONS
#ifndef NDEBUG
void initVulkan::debugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT* messenger)
{
	//setup debug messenger for all operations
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	populateDebugMessengerCreateInfo(&createInfo);
	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, messenger) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

void initVulkan::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* createInfo)
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
	createInfo->pfnUserCallback = debugUtilsMessengerCallback;
	createInfo->pUserData = nullptr; //optional pointer to user type
}

bool initVulkan::validationLayersSupported()
{
	//check if validation layer and selected optional layers are supported
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	VkLayerProperties* availableLayers = new VkLayerProperties[layerCount];
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);
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
			return false;
	}
	return true;
}

VkResult initVulkan::CreateDebugUtilsMessengerEXT(VkInstance instance,
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


VKAPI_ATTR VkBool32 VKAPI_CALL initVulkan::debugUtilsMessengerCallback(
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
