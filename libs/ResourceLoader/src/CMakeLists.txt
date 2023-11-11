add_library(ResourceLoader
    stb_image_impl.cpp
    vertex_model.cpp
    texture_loader.cpp
)
if(NOT NO_ASSIMP)
  target_sources(ResourceLoader PRIVATE
    model_loader.cpp
  )
endif()
if(NOT NO_FREETYPE)
  target_sources(ResourceLoader PRIVATE font_loader.cpp)
  add_dependencies(ResourceLoader freetype)
  target_link_libraries(ResourceLoader PRIVATE freetype)
endif()

add_dependencies(ResourceLoader OutfacingGraphics)
target_link_libraries(ResourceLoader PUBLIC OutfacingGraphics glm)

if(NOT NO_ASSIMP)
  target_link_libraries(ResourceLoader PUBLIC assimp)
endif()

target_include_directories(ResourceLoader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)	
