cmake_minimum_required(VERSION 3.10)

#customize
set(ProjectName "Vulkan-Environment")

#windows only
set(Lib "D:/noam4/Libraries/VS/lib/x64/")
set(Include "D:/noam4/Libraries/VS/include")

project(${ProjectName} VERSION 1.0)

set(CMAKE_CXX_STANDARD 14)

#packages
#find_package(Vulkan REQUIRED)

if (UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    find_package(X11 REQUIRED)
    link_libraries(${X11_LIBRARIES})
    add_definitions(-DVK_USE_PLATFORM_XLIB_KHR)
    find_package(glfw3 3.3 REQUIRED)
    find_package(Freetype REQUIRED)
endif()

#add executables
file(GLOB SOURCES src/*
  src/vulkan-render/*
  src/vulkan-render/resources/*
  src/vulkan-render/resources/model/*
  src/vulkan-render/parts/*)

#ignore any temps from emacs
list(FILTER SOURCES EXCLUDE REGEX ".*#+")

add_executable(${ProjectName} ${SOURCES})

#link
target_include_directories(${ProjectName} PUBLIC include/)
if(UNIX)
    target_include_directories(${ProjectName} PUBLIC ${GLFW_INCLUDE_DIRS}
                                                 PUBLIC ${VULKAN_INCLUDE_DIRS}
                                                 PUBLIC ${FREETYPE_INCLUDE_DIRS}
                                                 PUBLIC ${PROJECT_BINARY_DIR})
    target_link_libraries(${ProjectName} glfw
                                         Vulkan::Vulkan
                                         ${FREETYPE_LIBRARIES}
                                         assimp)
    #add assimp
    target_compile_definitions(${ProjectName} PUBLIC VK_USE_PLATFORM_XLIB_KHR)
else()
    target_include_directories(${ProjectName} PUBLIC ${Include})
    target_link_libraries(${ProjectName} ${Lib}glfw3.lib
                                         ${Lib}freetype.lib
                                         ${Lib}vulkan-1.lib
                                         ${Lib}assimp-vc140-mt.lib
                                         bufferoverflowU.lib)
    target_compile_definitions(${ProjectName} PUBLIC VK_USE_PLATFORM_WIN32_KHR)
endif()
