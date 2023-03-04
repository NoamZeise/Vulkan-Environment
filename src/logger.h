#ifndef ENV_VK_LOGGER_H
#define ENV_VK_LOGGER_H

#include <iostream>
#include <string>
#include <volk.h>

const char* getVkResultStr(VkResult result);

#define LOG(msg)				\
    std::cout << msg << std::endl;

#define LOG_ERROR(msg) \
    std::cerr << "Error: " << msg << std::endl;

#define GET_ERR_STRING(msg, vkresult) \
    std::string(msg) + " VK_RESULT: " + getVkResultStr(vkresult)


#endif
