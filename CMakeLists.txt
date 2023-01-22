cmake_minimum_required(VERSION 3.10)

project(Vulkan-Environment)

add_subdirectory(libs)
add_subdirectory(src)

if(BUILD_TESTS)
  add_subdirectory(test)
endif()

if(BUILD_DEMO)
  add_subdirectory(demo)
endif()

