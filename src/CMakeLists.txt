file(GLOB VK_RENDER_SOURCES
	  *
	  resources/*
	  parts/*
	  )

set(Vulkan-Render-Lib "VulkanEnvironment")

add_library(${Vulkan-Render-Lib} ${VK_RENDER_SOURCES})
target_include_directories(${Vulkan-Render-Lib} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../include/
)

add_dependencies(${Vulkan-Render-Lib} ResourceLoader OutfacingGraphics glfw)
target_link_libraries(${Vulkan-Render-Lib} PRIVATE ResourceLoader volk freetype)
target_link_libraries(${Vulkan-Render-Lib} PUBLIC OutfacingGraphics glfw glm)


target_include_directories(${Vulkan-Render-Lib} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}../include)
