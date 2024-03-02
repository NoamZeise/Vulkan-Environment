add_library(graphics-api animation.cpp)
add_dependencies(graphics-api glm)
target_link_libraries(graphics-api PUBLIC glm)
target_include_directories(graphics-api PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
