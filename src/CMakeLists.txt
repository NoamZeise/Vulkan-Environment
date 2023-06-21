file(GLOB VK_RENDER_SOURCES
	  *
	  resources/*
	  parts/*
	  parts/helper/*
	  render_structs/*
	  render_structs/swapchain/*
	  )

set(Vulkan-Render-Lib "VulkanEnvironment")

add_library(${Vulkan-Render-Lib} ${VK_RENDER_SOURCES})

add_dependencies(${Vulkan-Render-Lib} ResourceLoader OutfacingGraphics glfw volk)

target_link_libraries(${Vulkan-Render-Lib} PRIVATE 
  ResourceLoader )

if(NOT NO_FREETYPE)
  add_dependencies(${Vulkan-Render-Lib} freetype)
  target_link_libraries(${Vulkan-Render-Lib} PRIVATE freetype)
endif()

target_link_libraries(${Vulkan-Render-Lib} PUBLIC 
  OutfacingGraphics 
  glfw
  volk
)

if(VKENV_BUILD_STATIC)
   if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
     target_link_options(${Vulkan-Render-Lib} PUBLIC -static-libgcc -static-libstdc++)
   endif()
endif()

target_include_directories(${Vulkan-Render-Lib} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include/)
