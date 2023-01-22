add_subdirectory(OutfacingGraphics)
add_subdirectory(ResourceLoader)
add_subdirectory(glfw)
add_subdirectory(glm)
add_subdirectory(volk)
if(NOT NO_ASSIMP)
  OPTION( ASSIMP_NO_EXPORT
    "Disable Assimp's export functionality."
    OFF
  )
  set(ASSIMP_NO_EXPORT ON)
  OPTION( ASSIMP_INSTALL
    "Disable this if you want to use assimp as a submodule."
    ON
  )
  set(ASSIMP_INSTALL OFF)
  OPTION(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT "default value of all ASSIMP_BUILD_XXX_IMPORTER values" TRUE)
  set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT FALSE)
  OPTION ( ASSIMP_BUILD_TESTS
    "If the test suite for Assimp is built in addition to the library."
    ON
  )
  set(ASSIMP_BUILD_TESTS FALSE)
  set(ASSIMP_BUILD_FBX_IMPORTER TRUE)
  add_subdirectory(assimp)
endif()
