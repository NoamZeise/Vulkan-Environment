add_library(resource-loader
    stb_image_impl.cpp
    vertex_model.cpp
    texture_loader.cpp
    model_loader.cpp
    assimp_loader.cpp
)
target_sources(resource-loader PRIVATE font_loader.cpp)
if(NOT NO_FREETYPE)
  add_dependencies(resource-loader freetype)
  target_link_libraries(resource-loader PRIVATE freetype)
endif()

add_dependencies(resource-loader graphics-api)
target_link_libraries(resource-loader PUBLIC graphics-api)

if(NOT NO_ASSIMP)
  target_link_libraries(resource-loader PUBLIC assimp)
endif()

target_include_directories(resource-loader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)	
