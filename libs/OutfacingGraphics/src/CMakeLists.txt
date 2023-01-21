add_library(OutfacingGraphics resources/model/model_animation.cpp)
target_include_directories(OutfacingGraphics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
  target_include_directories(OutfacingGraphics PRIVATE "D:/noam4/Libraries/VS/include")
endif()
