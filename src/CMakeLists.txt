file(GLOB VK_RENDER_SOURCES
	  *
	  resources/*
	  parts/*
	  parts/helper/*
	  render_structs/*
	  )

set(Vulkan-Render-Lib "VulkanEnvironment")

add_library(${Vulkan-Render-Lib} ${VK_RENDER_SOURCES})

add_dependencies(${Vulkan-Render-Lib} resource-loader graphics-api glfw volk)

target_link_libraries(${Vulkan-Render-Lib} PRIVATE resource-loader)

if(NOT NO_FREETYPE)
  add_dependencies(${Vulkan-Render-Lib} freetype)
  target_link_libraries(${Vulkan-Render-Lib} PRIVATE freetype)
endif()

target_link_libraries(${Vulkan-Render-Lib} PUBLIC 
  graphics-api
  glfw
  volk
)

target_include_directories(${Vulkan-Render-Lib} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include/)
