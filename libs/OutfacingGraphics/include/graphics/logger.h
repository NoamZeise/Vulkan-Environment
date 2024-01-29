#ifndef GRAPHICS_ENV_LOGGER_H
#define GRAPHICS_ENV_LOGGER_H

#include <iostream>
#include <string>
#include <glm/glm.hpp>

#ifndef NDEBUG
#define LOG(msg) std::cout << msg << std::endl;
#else
#define LOG(msg)
#endif

#define LOG_LINE() LOG("---------------------");

#define LOG_ERROR(msg) LOG_CERR(msg, "Error: ");

#define LOG_CERR(msg, severity) std::cerr << severity << msg << std::endl;

inline std::string mat4_to_str(glm::mat4 mat4) {
    std::string t = "[";
    for(int r = 0; r < 4; r++) {
	for(int c = 0; c < 4; c++) {
	    t += std::to_string(mat4[r][c]) + ", ";
	}
	t += "\n";
    }
    t+="]";
    return t;
}

#endif
