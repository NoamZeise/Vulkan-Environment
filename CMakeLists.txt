cmake_minimum_required(VERSION 3.15)

project(Vulkan-Environment)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NO_ASSIMP)
  add_compile_definitions(NO_ASSIMP)
endif()

if(NO_FREETYPE)
  add_compile_definitions(NO_FREETYPE)
endif()

if(VKENV_BUILD_STATIC)
  if(MSVC)
    set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  endif()
endif()

add_subdirectory(libs)
add_subdirectory(api)
add_subdirectory(resource-loader)
add_subdirectory(src)

if(VKENV_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

