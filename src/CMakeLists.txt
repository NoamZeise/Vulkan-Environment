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

add_dependencies(${Vulkan-Render-Lib} ResourceLoader OutfacingGraphics)
target_link_libraries(${Vulkan-Render-Lib} PRIVATE ResourceLoader)
target_link_libraries(${Vulkan-Render-Lib} PUBLIC OutfacingGraphics)

if(UNIX)
    target_include_directories(${Vulkan-Render-Lib} PUBLIC ${FREETYPE_INCLUDE_DIRS})
    target_link_libraries(${Vulkan-Render-Lib} PRIVATE
      glfw
      ${FREETYPE_LIBRARIES}
      ${CMAKE_DL_LIBS})
else()
    target_include_directories(${Vulkan-Render-Lib} PRIVATE "D:/noam4/Libraries/VS/include")
    target_link_libraries(${Vulkan-Render-Lib} PRIVATE
      ${Lib}glfw3.lib
      ${Lib}freetype.lib
      ${Lib}vulkan-1.lib
      bufferoverflowU.lib)
endif()


#target_include_directories(${Vulkan-Render-Lib} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
