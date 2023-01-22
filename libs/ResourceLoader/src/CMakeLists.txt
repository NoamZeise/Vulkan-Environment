add_library(ResourceLoader
  resources/stb_image_impl.cpp
  resources/model_loader.cpp
)

add_dependencies(ResourceLoader OutfacingGraphics)
target_link_libraries(ResourceLoader PUBLIC OutfacingGraphics glm assimp)
#target_link_libraries(ResourceLoader PRIVATE assimp)

target_include_directories(ResourceLoader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
