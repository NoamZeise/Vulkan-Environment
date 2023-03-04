#include "logger.h"

const char* getVkResultStr(VkResult result) {
    switch(result) {
    case VK_SUCCESS:
	return "Success";
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
    default:
	return "Unknown Error";
    }
}
