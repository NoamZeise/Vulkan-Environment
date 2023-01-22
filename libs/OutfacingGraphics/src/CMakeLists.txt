add_library(OutfacingGraphics resources/model/model_animation.cpp)

target_link_libraries(OutfacingGraphics PUBLIC glm)
target_include_directories(OutfacingGraphics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
