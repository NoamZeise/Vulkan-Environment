cmake_minimum_required(VERSION 3.10)

project(Vulkan-Environment)

if(NO_ASSIMP)
  add_compile_definitions(NO_ASSIMP)
endif()

if(NO_FREETYPE)
  add_compile_definitions(NO_FREETYPE)
endif()

if(STATIC_FLAG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
endif()

add_subdirectory(libs)
add_subdirectory(src)

if(BUILD_TESTS)
  add_subdirectory(test)
endif()

if(BUILD_DEMO)
  add_subdirectory(demo)
endif()

