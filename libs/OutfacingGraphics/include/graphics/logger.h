#ifndef GRAPHICS_ENV_LOGGER_H
#define GRAPHICS_ENV_LOGGER_H

#include <iostream>

#ifndef NDEBUG
#define LOG(msg) std::cout << msg << std::endl;
#else
#define LOG(msg)
#endif

#define LOG_LINE() LOG("---------------------");

#define LOG_ERROR(msg) std::cerr << "Error: " << msg << std::endl;

#endif
