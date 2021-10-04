cmake_minimum_required(VERSION 3.10)

# set the project name
project(2D-Vulkan-Environment VERSION 1.0)

set (CMAKE_CXX_STANDARD 11)

#packages
find_package(Vulkan REQUIRED)

if (UNIX)
    find_package(X11 REQUIRED)
    link_libraries(${X11_LIBRARIES})
    add_definitions(-DVK_USE_PLATFORM_XLIB_KHR)
    find_package(glfw3 3.3 REQUIRED)

    find_package(Freetype REQUIRED)
endif()



if(MSVC)
  set(CMAKE_CXX_FLAGS_DEBUG_INIT "/D_DEBUG /MTd /Zi /Ob0 /Od /RTC1")
  set(CMAKE_CXX_FLAGS_MINSIZEREL_INIT     "/MT /O1 /Ob1 /D NDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE_INIT        "/MT /O2 /Ob2 /D NDEBUG")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "/MT /Zi /O2 /Ob1 /D NDEBUG")
else()
  SET(CMAKE_CXX_FLAGS "-O2 -std=c++11")
  SET(CMAKE_EXE_LINKER_FLAGS "-v")
endif()


#add executables
file(GLOB SOURCES src/*)
add_executable(2D-Vulkan-Environment ${SOURCES})


#link
if(UNIX)
    target_include_directories(2D-Vulkan-Environment PUBLIC ${GLFW_INCLUDE_DIRS}
                                                 PUBLIC ${VULKAN_INCLUDE_DIRS}
                                                 PUBLIC ${FREETYPE_INCLUDE_DIRS}
                                                 PUBLIC ${PROJECT_BINARY_DIR})
    target_link_libraries(2D-Vulkan-Environment glfw)
    target_link_libraries(2D-Vulkan-Environment Vulkan::Vulkan)
    target_link_libraries(2D-Vulkan-Environment ${FREETYPE_LIBRARIES})
else()
    target_include_directories(2D-Vulkan-Environment PUBLIC D:/noam4/Libraries/VS/include)
    target_link_libraries(2D-Vulkan-Environment libs/glfw3)
    target_link_libraries(2D-Vulkan-Environment libs/freetyped)
    target_link_libraries(2D-Vulkan-Environment libs/vulkan-1)
endif()
