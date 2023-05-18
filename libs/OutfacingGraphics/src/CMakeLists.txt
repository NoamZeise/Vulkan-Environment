add_library(OutfacingGraphics animation.cpp)
target_link_libraries(OutfacingGraphics PUBLIC glm)
target_include_directories(OutfacingGraphics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
