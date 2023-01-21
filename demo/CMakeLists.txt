add_executable(VKenvDemo demo.cpp)

target_link_libraries(VKenvDemo VulkanEnvironment)

add_custom_command(TARGET VKenvDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../resources"
        $<TARGET_FILE_DIR:VKenvDemo>)
