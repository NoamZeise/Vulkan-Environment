#ifndef ENV_VK_LOGGER_H
#define ENV_VK_LOGGER_H

#include <logger.h>
#include <string>
#include <volk.h>

const char* getVkResultStr(VkResult result);

#define GET_ERR_STRING(msg, vkresult)					\
    std::string(msg) + " VK_RESULT: " + getVkResultStr(vkresult)

#define LOG_ERR_TYPE(msg, err)			\
    LOG_ERROR(GET_ERR_STRING(msg, err))


#endif
