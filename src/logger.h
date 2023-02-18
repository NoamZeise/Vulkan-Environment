#ifndef ENV_VK_LOGGER_H
#define ENV_VK_LOGGER_H


#define LOG(msg)				\
    std::cout << msg << std::endl;

#define LOG_ERROR(msg) \
    std::cerr << "Error: " << msg << std::endl;

#endif
