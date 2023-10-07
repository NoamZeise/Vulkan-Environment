#ifndef VK_ENV_LOGGER_H
#define VK_ENV_LOGGER_H

#include <graphics/logger.h>
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


//! Both Macros require a 'VkResult result' var to be in scope

#define returnOnErr(result_expr)					\
    result = result_expr;						\
  if (result != VK_SUCCESS)						\
      return result;

#define msgAndReturnOnErr(result_expr, msg)				\
    result = result_expr;						\
  if (result != VK_SUCCESS) {						\
      LOG_ERR_TYPE(msg, result);					\
      return result;							\
  }

#endif
