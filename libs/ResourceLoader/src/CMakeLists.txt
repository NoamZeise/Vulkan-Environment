add_library(ResourceLoader
  resources/stb_image_impl.cpp
  resources/model_loader.cpp
)

add_dependencies(ResourceLoader OutfacingGraphics)
target_link_libraries(ResourceLoader PUBLIC OutfacingGraphics)

if(UNIX)
  target_link_libraries(ResourceLoader PRIVATE assimp)
elseif(WIN32)
  target_include_directories(ResourceLoader PRIVATE "D:/noam4/Libraries/VS/include")
  target_link_libraries(ResourceLoader PRIVATE "d:/noam4/Libraries/VS/lib/x64/assimp-vc140-mt.lib")
endif()


target_include_directories(ResourceLoader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
