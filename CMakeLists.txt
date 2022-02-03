cmake_minimum_required(VERSION 3.10)

set(ProjectName "Vulkan-Environment")
# set the project name
project(${ProjectName} VERSION 1.0)

SET(CMAKE_CXX_STANDARD 11)

#packages
find_package(Vulkan REQUIRED)

if (UNIX)
    find_package(X11 REQUIRED)
    link_libraries(${X11_LIBRARIES})
    add_definitions(-DVK_USE_PLATFORM_XLIB_KHR)
    find_package(glfw3 3.3 REQUIRED)
    find_package(Freetype REQUIRED)
    SET(CMAKE_CXX_FLAGS "-O2 -std=c++11")
    SET(CMAKE_EXE_LINKER_FLAGS "-v")

elseif (MSVC)
  set(CMAKE_CXX_FLAGS_DEBUG_INIT "/D_DEBUG /MTd /Zi /Ob0 /Od /RTC1")
  set(CMAKE_CXX_FLAGS_MINSIZEREL_INIT     "/MT /O1 /Ob1 /D NDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE_INIT        "/MT /O2 /Ob2 /D NDEBUG")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "/MT /Zi /O2 /Ob1 /D NDEBUG")
endif()


#add executables
file(GLOB SOURCES src/*)
file(GLOB VULKAN-RENDER src/vulkan-render/*)
add_executable(${ProjectName} ${SOURCES}
                              ${VULKAN-RENDER}
                              ${PROJ_INCLUDE})

#link
target_include_directories(${ProjectName} PUBLIC include/)
if(UNIX)
    target_include_directories(${ProjectName} PUBLIC ${GLFW_INCLUDE_DIRS}
                                                 PUBLIC ${VULKAN_INCLUDE_DIRS}
                                                 PUBLIC ${FREETYPE_INCLUDE_DIRS}
                                                 PUBLIC ${PROJECT_BINARY_DIR})
    target_link_libraries(${ProjectName} glfw)
    target_link_libraries(${ProjectName} Vulkan::Vulkan)
    target_link_libraries(${ProjectName} ${FREETYPE_LIBRARIES})
    #add assimp
elseif(MSVC)
    target_include_directories(${ProjectName} PUBLIC D:/noam4/Libraries/VS/include)
    target_link_libraries(${ProjectName} libs/glfw3)
    target_link_libraries(${ProjectName} libs/freetyped)
    target_link_libraries(${ProjectName} libs/vulkan-1)
    target_link_libraries(${ProjectName} libs/assimp-vc140-mt)
endif()
