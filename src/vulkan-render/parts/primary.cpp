#include "primary.h"


namespace part
{

namespace create
{

#ifndef NDEBUG

const std::array<const char*, 1> OPTIONAL_LAYERS = {
		"VK_LAYER_KHRONOS_validation"
};
const std::array<VkValidationFeatureEnableEXT, 2> VALIDATION_LAYER_FEATURES = {
	VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
	VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT

};

bool _validationLayersSupported();
void _populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* createInfo);
VkResult _createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
VKAPI_ATTR VkBool32 VKAPI_CALL _debugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

#endif // NDEBUG

const std::array<const char*, 1> REQUESTED_DEVICE_EXTENSIONS = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

void Instance(VkInstance* instance)
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

void Device(VkInstance instance, Base* base, VkSurfaceKHR surface)
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

//DEBUG FUNCTIONS
#ifndef NDEBUG
void DebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT* messenger)
{
	//setup debug messenger for all operations
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	_populateDebugMessengerCreateInfo(&createInfo);
	if (_createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, messenger) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

void _populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* createInfo)
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

bool _validationLayersSupported()
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

VkResult _createDebugUtilsMessengerEXT(VkInstance instance,
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



VKAPI_ATTR VkBool32 VKAPI_CALL _debugUtilsMessengerCallback(
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

}

#ifndef NDEBUG
namespace destroy
{

void DebugMessenger(VkInstance instance,VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

}
#endif

}
