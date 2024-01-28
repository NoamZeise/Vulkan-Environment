cmake_minimum_required(VERSION 3.15)

project(Vulkan-Environment)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#project vars:
#         -BUILD_DEMO
#         -BUILD_TESTS
#         -NO_FREETYPE
#         -NO_ASSIMP
#         -VKENV_BUILD_STATIC
#         -BUILD_ASSIMP_STATIC

if(NO_ASSIMP)
  add_compile_definitions(NO_ASSIMP)
endif()

if(NO_FREETYPE)
  add_compile_definitions(NO_FREETYPE)
endif()

if(VKENV_BUILD_STATIC)
  set(BUILD_ASSIMP_STATIC true)
  set(BUILD_SHARED_LIBS OFF)
  if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  endif()
endif()

add_subdirectory(libs)
add_subdirectory(src)

if(VKENV_BUILD_TESTS)
  add_subdirectory(test)
endif()

if(VKENV_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

