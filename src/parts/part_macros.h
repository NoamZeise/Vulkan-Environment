#ifndef VK_ENV_PARTS_MACROS_H
#define VK_ENV_PARTS_MACROS_H

//! Both Macros require a 'VkResult result' var to be in scope

#include <iostream>

#include "../logger.h"

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
