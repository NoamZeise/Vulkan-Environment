add_subdirectory(OutfacingGraphics)
add_subdirectory(ResourceLoader)

# glfw setup
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" ON)
set(GLFW_BUILD_EXAMPLES FALSE)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" ON)
set(GLFW_BUILD_TESTS FALSE)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" ON)
set(GLFW_BUILD_DOCS FALSE)
option(GLFW_INSTALL "Generate installation target" ON)
set(GLFW_INSTALL FALSE)
add_subdirectory(glfw)


add_subdirectory(glm)


add_subdirectory(volk)

if(NOT NO_ASSIMP)
  OPTION( ASSIMP_NO_EXPORT "Disable Assimp's export functionality." OFF)
  set(ASSIMP_NO_EXPORT ON)
  OPTION( ASSIMP_INSTALL "Disable this if you want to use assimp as a submodule." ON )
  set(ASSIMP_INSTALL OFF)
  OPTION ( ASSIMP_BUILD_TESTS "If the test suite for Assimp is built in addition to the library." ON)
  set(ASSIMP_BUILD_TESTS OFF)
  OPTION(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT "default value of all ASSIMP_BUILD_XXX_IMPORTER values" TRUE)
  set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT FALSE)
  set(ASSIMP_BUILD_FBX_IMPORTER TRUE)
  set(ASSIMP_BUILD_OBJ_IMPORTER TRUE)
  add_subdirectory(assimp)
endif()
