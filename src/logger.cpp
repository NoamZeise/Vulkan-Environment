#include "logger.h"

const char* getVkResultStr(VkResult result) {
    switch(result) {
    case VK_SUCCESS:
	return "Success";
    case VK_TIMEOUT:
	return "Timeout";
    case VK_NOT_READY:
	return "Not ready";
    case VK_SUBOPTIMAL_KHR:
	return "Suboptimal KHR";
    case VK_ERROR_INITIALIZATION_FAILED:
	return "Initialization Failed";
    case VK_ERROR_OUT_OF_DATE_KHR:
	return "Out of date KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
	return "Native window in use KHR";
    case VK_ERROR_FRAGMENTATION:
	return "Fragmentaion";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
	return "Invalid External Handle";
    case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:
	return "Invalid Device Address EXT";
    case VK_ERROR_OUT_OF_POOL_MEMORY:
	return "Out of Pool Memeory";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
	return "Out of Device Memory";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
	return "Out of Host Memory";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
	return "Extension not present";
    case VK_ERROR_FEATURE_NOT_PRESENT:
	return "Feature not present";
    case VK_ERROR_FRAGMENTED_POOL:
	return "Fragmented Pool";
    case VK_ERROR_DEVICE_LOST:
	return "device lost";
    case VK_ERROR_SURFACE_LOST_KHR:
	return "Surface lost";
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
	return "Full screen exclusive mode lost";
    case VK_ERROR_UNKNOWN:
	return "Unknown Error";
    default:
	return "Unrecognised Error";
    }
}

std::string resultMessageString(std::string message, VkResult result) {
    return message + " VK_RESULT: " + getVkResultStr(result);
}


void checkResultAndThrow(VkResult result, std::string message) {
    if(result != VK_SUCCESS) {
	std::string errMsg = resultMessageString(message, result);
	throw std::runtime_error(errMsg);
    }
}
