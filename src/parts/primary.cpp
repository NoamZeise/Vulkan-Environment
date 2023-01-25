#include "primary.h"

#include <GLFW/glfw3.h>
#include <config.h>

#include <array>
#include <set>
#include <cstring>
#include <iostream>
#include <stdint.h>
#include <vulkan/vulkan_core.h>

#include "part_macros.h"
#include "helper/debug.h"

namespace part {

namespace create {

#ifndef NDEBUG

const std::array<const char *, 1> OPTIONAL_LAYERS = {
    "VK_LAYER_KHRONOS_validation"};
    
const std::array<VkValidationFeatureEnableEXT, 2> VALIDATION_LAYER_FEATURES = {
    VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
    VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT

};  
#else
const std::array<const char *, 1> OPTIONAL_LAYERS = {};    
#endif // NDEBUG
    
bool checkRequiredLayersSupported();

    
const std::array<const char *, 1> REQUESTED_DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

VkResult Instance(VkInstance *instance) {
  VkResult result = VK_SUCCESS;
  VkInstanceCreateInfo instanceCreateInfo{
      VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};

  // app info
  VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.pApplicationName = "Vulkan App";
  appInfo.pEngineName = "No Engine";
  appInfo.apiVersion = VK_API_VERSION_1_0;
  appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  instanceCreateInfo.pApplicationInfo = &appInfo;

  // check required extensions are supported
  uint32_t requiredExtensionsCount = 0;
  const char **requiredExtensions =
      glfwGetRequiredInstanceExtensions(&requiredExtensionsCount);
  std::vector<const char *> extensions(
      requiredExtensions, requiredExtensions + requiredExtensionsCount);
#ifndef NDEBUG
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
  instanceCreateInfo.enabledExtensionCount =
      static_cast<uint32_t>(extensions.size());
  instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

  if (!checkRequiredLayersSupported()) {
    std::cerr << "ERROR: extension layers that were requested aren't "
                 "supported by the chosen device. Make sure you have the Vulkan SDK installed, and "
                 "that the requested layers are available.\n";
    return VK_ERROR_FEATURE_NOT_PRESENT;
  }
  #ifndef NDEBUG
  instanceCreateInfo.enabledLayerCount =
      static_cast<uint32_t>(OPTIONAL_LAYERS.size());
  instanceCreateInfo.ppEnabledLayerNames = OPTIONAL_LAYERS.data();

  VkValidationFeaturesEXT validationFeatures{
      VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
  validationFeatures.enabledValidationFeatureCount =
      static_cast<uint32_t>(VALIDATION_LAYER_FEATURES.size());
  validationFeatures.pEnabledValidationFeatures =
      VALIDATION_LAYER_FEATURES.data();
  instanceCreateInfo.pNext = &validationFeatures; // call after create instance

  // setup debug messenger for just the creation of an instance
  VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
  populateDebugMessengerCreateInfo(&debugMessengerInfo);
  validationFeatures.pNext =
      &debugMessengerInfo; // call after validation feature creation
#else
  instanceCreateInfo.enabledLayerCount = 0;
  instanceCreateInfo.pNext = nullptr;
#endif

  returnOnErr(vkCreateInstance(&instanceCreateInfo, nullptr, instance));
  volkLoadInstance(*instance);
  return VK_SUCCESS;
}


VkResult checkReqeustedExtensionsAreSupported(VkPhysicalDevice physicalDevice) {
  VkResult result = VK_SUCCESS;
  uint32_t extensionCount;
  msgAndReturnOnErr(vkEnumerateDeviceExtensionProperties(physicalDevice,
							 nullptr,
							 &extensionCount,
							 nullptr),
		    "failed to find device extensions count");
  std::vector<VkExtensionProperties> deviceExtensions(extensionCount);
  msgAndReturnOnErr(vkEnumerateDeviceExtensionProperties(physicalDevice,
							 nullptr,
							 &extensionCount,
							 deviceExtensions.data()),
		    "failed to find device extenions");
	    
  for (const auto &extension : REQUESTED_DEVICE_EXTENSIONS) {
      bool found = false;
      for (const auto &supported : deviceExtensions) {
	  // std::cout << supported.extensionName  << std::endl;
	  if (std::strcmp(extension, supported.extensionName) == 0) {
	      found = true;
	      break;
	  }
      }
      if (!found) {
	  std::cerr << "Error: Device does not support requested extention\n";
	  return VK_ERROR_EXTENSION_NOT_PRESENT;
      }
  }
  return result;
}

std::vector<VkDeviceQueueCreateInfo> fillQueueFamiliesCreateInfo(std::set<uint32_t> uniqueQueueFamilies) {
  std::vector<VkDeviceQueueCreateInfo> queueInfos(uniqueQueueFamilies.size());
  float queuePriority = 1.0f;
  int familyCount = 0;
  for (uint32_t familyIndex : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueCreateInfo.queueFamilyIndex = familyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    queueInfos[familyCount] = queueCreateInfo;
    familyCount++;
  }
  return queueInfos;
}
    

VkResult choosePhysicalDevice(VkInstance instance, VkSurfaceKHR surface,
                              VkPhysicalDevice *physicalDevice,
                              uint32_t *graphicsPresentQueueFamilyId);

bool checkIfPhysicalDeviceIsValid(
    VkPhysicalDevice candidateDevice, VkSurfaceKHR surface,
    const std::vector<VkQueueFamilyProperties> &queueFamilyProps,
    uint32_t *graphicsPresentQueueID);

VkResult Device(VkInstance instance, DeviceState *deviceState, VkSurfaceKHR surface, EnabledFeatures requestFeatures) {
    VkResult result = VK_SUCCESS;
    returnOnErr(choosePhysicalDevice(instance,
				     surface,
				     &deviceState->physicalDevice,
				     &deviceState->queue.graphicsPresentFamilyIndex));

  // create logical device

  VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};

  std::vector<VkDeviceQueueCreateInfo> queueInfos = fillQueueFamiliesCreateInfo(
			    { deviceState->queue.graphicsPresentFamilyIndex});
  deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
  deviceInfo.pQueueCreateInfos = queueInfos.data();

  msgAndReturnOnErr(checkReqeustedExtensionsAreSupported(deviceState->physicalDevice),
		    "The selected device did not support all of the requested extensions");
  deviceInfo.enabledExtensionCount =
      static_cast<uint32_t>(REQUESTED_DEVICE_EXTENSIONS.size());
  deviceInfo.ppEnabledExtensionNames = REQUESTED_DEVICE_EXTENSIONS.data();

  VkPhysicalDeviceFeatures availableDeviceFeatures;
  vkGetPhysicalDeviceFeatures(deviceState->physicalDevice, &availableDeviceFeatures);
  // enable optional device features
  VkPhysicalDeviceFeatures chosenDeviceFeatures{};
  if (availableDeviceFeatures.samplerAnisotropy && requestFeatures.samplerAnisotropy) {
    chosenDeviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceState->features.samplerAnisotropy = true;
  }
  if (availableDeviceFeatures.sampleRateShading && requestFeatures.sampleRateShading) {
      chosenDeviceFeatures.sampleRateShading = VK_TRUE;
      deviceState->features.sampleRateShading = true;
  }

  deviceInfo.pEnabledFeatures = &chosenDeviceFeatures;

#ifndef NDEBUG
  deviceInfo.enabledLayerCount = static_cast<uint32_t>(OPTIONAL_LAYERS.size());
  deviceInfo.ppEnabledLayerNames = OPTIONAL_LAYERS.data();
#else
  deviceInfo.enabledLayerCount = 0;
#endif // NDEBUG

  returnOnErr(vkCreateDevice(deviceState->physicalDevice, &deviceInfo, nullptr,
			     &deviceState->device))

  volkLoadDevice(deviceState->device);
  // get queue handles for graphics and present
  vkGetDeviceQueue(deviceState->device,
                   deviceState->queue.graphicsPresentFamilyIndex, 0,
                   &deviceState->queue.graphicsPresentQueue);
  return result;
}

    
#ifndef NDEBUG
VkResult DebugMessenger(VkInstance instance,
                        VkDebugUtilsMessengerEXT *messenger) {
  VkResult result = VK_SUCCESS;
  // setup debug messenger for all operations
  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  populateDebugMessengerCreateInfo(&createInfo);
  returnOnErr(
      createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, messenger));
  return result;
}
#endif


bool checkRequiredLayersSupported() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
  for (auto layer : OPTIONAL_LAYERS) {
    bool layerSupported = false;
    for (size_t i = 0; i < layerCount; i++) {
      if (std::strcmp(layer, availableLayers[i].layerName) == 0) {
        layerSupported = true;
        break;
      }
    }
    if (!layerSupported) {
      return false;
    }
  }
  return true;
}

    
bool checkIfPhysicalDeviceIsValid(VkPhysicalDevice candidateDevice,
				  VkSurfaceKHR surface,
				  const std::vector<VkQueueFamilyProperties> &queueFamilyProps,
				  uint32_t *graphicsPresentQueueID) {
    VkBool32 graphicQueueSupport = VK_FALSE;
    VkBool32 presentQueueSupport = VK_FALSE;
    uint32_t graphicsPresent;
    for (uint32_t j = 0; j < queueFamilyProps.size(); j++) {
	vkGetPhysicalDeviceSurfaceSupportKHR(candidateDevice, j, surface,
					     &presentQueueSupport);
	if (queueFamilyProps[j].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
	    presentQueueSupport) {
	    graphicQueueSupport = VK_TRUE;
	    graphicsPresent = j;
	}
    }
    *graphicsPresentQueueID = graphicsPresent;
    return graphicQueueSupport && presentQueueSupport;
}

    
VkResult choosePhysicalDevice(VkInstance instance,
			      VkSurfaceKHR surface,
			      VkPhysicalDevice *physicalDevice,
			      uint32_t *graphicsPresentQueueFamilyId) {
  VkResult result;
  uint32_t deviceCount;
  msgAndReturnOnErr(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr), "Failed to get device count!");
  if (deviceCount < 1) {
      std::cerr << "Error: No Physical Devices Found\n";
      return VK_ERROR_INITIALIZATION_FAILED;
  }

  std::vector<VkPhysicalDevice> gpus(deviceCount);
  msgAndReturnOnErr(vkEnumeratePhysicalDevices(instance, &deviceCount, gpus.data()), "Failed to get device list!");

  bool foundSuitable = false;
  for (size_t i = 0; i < gpus.size(); i++) {
      VkPhysicalDeviceProperties deviceProperties;
      vkGetPhysicalDeviceProperties(gpus[i], &deviceProperties);
      
      uint32_t queueFamilyCount = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(gpus[i], &queueFamilyCount, nullptr);
      std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
      vkGetPhysicalDeviceQueueFamilyProperties(gpus[i], &queueFamilyCount, queueFamilies.data());
      if (checkIfPhysicalDeviceIsValid(gpus[i], surface, queueFamilies, graphicsPresentQueueFamilyId)) {
	  *physicalDevice = gpus[i];
	  foundSuitable = true;
	  if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	      break;
      }
  }
  if (!foundSuitable)
  {
      std::cerr << "Error: Failed to find device which supports graphics and present queues\n";
      return VK_ERROR_FEATURE_NOT_PRESENT;
  }
  return result;
}
} // namespace create

#ifndef NDEBUG
namespace destroy {

void DebugMessenger(VkInstance instance,
                    VkDebugUtilsMessengerEXT debugMessenger,
                    const VkAllocationCallbacks *pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

} // namespace destroy
#endif

} // namespace part
