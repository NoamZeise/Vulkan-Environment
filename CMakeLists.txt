cmake_minimum_required(VERSION 3.10)

project(Vulkan-Environment)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#project vars:
#         -BUILD_DEMO
#         -BUILD_TESTS
#         -NO_FREETYPE
#         -NO_ASSIMP
#         -STATIC_FLAG
#         -BUILD_ASSIMP_STATIC


if(NO_ASSIMP)
  add_compile_definitions(NO_ASSIMP)
endif()

if(NO_FREETYPE)
  add_compile_definitions(NO_FREETYPE)
endif()

if(STATIC_FLAG)
  if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
  endif()
endif()

add_subdirectory(libs)
add_subdirectory(src)

if(BUILD_TESTS)
  add_subdirectory(test)
endif()

if(BUILD_DEMO)
  add_subdirectory(demo)
endif()

