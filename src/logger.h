#ifndef VK_ENV_LOGGER_H
#define VK_ENV_LOGGER_H

#include <logger.h>
#include <string>
#include <volk.h>

/// Return a string indicating the type of the result
const char *getVkResultStr(VkResult result);

/// Return a string with the supplied message and the type of VkResult
std::string resultMessageString(std::string message, VkResult result);

/// Log a message and a result type to std::err
#define LOG_ERR_TYPE(msg, err)			\
    LOG_ERROR(resultMessageString(msg, err))

/// If VkResult is not a success,
/// log an error with the result type and throw an exception
void checkResultAndThrow(VkResult result, std::string message);

#endif
