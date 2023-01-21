cmake_minimum_required(VERSION 3.10)

project(Vulkan-Environment)

if(UNIX)
  find_package(glfw3 3.3 REQUIRED)
  find_package(Freetype REQUIRED)
endif()

add_subdirectory(src)

if(BUILD_TESTS)
  add_subdirectory(test)
endif()
