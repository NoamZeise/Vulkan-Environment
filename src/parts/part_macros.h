#ifndef PART_MACROS_H
#define PART_MACROS_H

//! Both Macros require a 'VkResult result' var to be in scope, this is to avoid
//! multiple defs for multiple calls in one function

#include <iostream>

#define returnOnErr(result_expr)					       \
  result = result_expr;                                                        \
  if (result != VK_SUCCESS)                                                    \
    return result;

#define msgAndReturnOnErr(result_expr, msg)                                    \
  result = result_expr;                                                        \
  if (result != VK_SUCCESS) {					               \
    std::cerr << "Error: " << msg << std::endl;                                \
    return result;                                                             \
  }

#endif
