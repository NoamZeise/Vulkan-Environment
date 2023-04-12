if(NO_ASSIMP)
  add_library(ResourceLoader
    resources/stb_image_impl.cpp
    resource_loader/vertex_model.cpp
  )
else()
  add_library(ResourceLoader
    resources/stb_image_impl.cpp
    resources/model_loader.cpp
    resource_loader/vertex_model.cpp
  )
endif()

add_dependencies(ResourceLoader OutfacingGraphics)
target_link_libraries(ResourceLoader PUBLIC OutfacingGraphics glm)

if(NOT NO_ASSIMP)
  target_link_libraries(ResourceLoader PUBLIC assimp)
endif()
#target_link_libraries(ResourceLoader PRIVATE assimp)

target_include_directories(ResourceLoader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
