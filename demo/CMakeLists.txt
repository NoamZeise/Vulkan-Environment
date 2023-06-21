add_executable(VKenvDemo demo.cpp)

target_link_libraries(VKenvDemo VulkanEnvironment)

if(UNIX) # want linux binary to check current dir for libraries
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath='${ORIGIN}'")
    if(VKENV_BUILD_STATIC)
      target_link_libraries(VKenvDemo -static-libgcc -static-libstdc++)
    endif()
  endif()
endif()

add_custom_command(TARGET VKenvDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../resources"
        $<TARGET_FILE_DIR:VKenvDemo>)

if(NOT NO_ASSIMP)
  if(NOT BUILD_ASSIMP_STATIC)	
    add_custom_command(TARGET VKenvDemo POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
      $<TARGET_FILE_DIR:assimp>
      $<TARGET_FILE_DIR:VKenvDemo>)
  endif()
endif()

if(NOT NO_FREETYPE)
  if(NOT VKENV_BUILD_STATIC)
    add_custom_command(TARGET VKenvDemo POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
      $<TARGET_FILE_DIR:freetype>
      $<TARGET_FILE_DIR:VKenvDemo>)
  endif()
endif()

if(NOT VKENV_BUILD_STATIC)
  add_custom_command(TARGET VKenvDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    $<TARGET_FILE_DIR:glfw>
    $<TARGET_FILE_DIR:VKenvDemo>)
endif()
