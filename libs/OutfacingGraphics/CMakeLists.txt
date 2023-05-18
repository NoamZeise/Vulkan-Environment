# The publicly accessable functions that can be used by the graphics backends,
# or by any projects using the backends.
cmake_minimum_required(VERSION 3.10)

project(OutfacingGraphics)

add_subdirectory(src)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  add_subdirectory(demo)
endif()
