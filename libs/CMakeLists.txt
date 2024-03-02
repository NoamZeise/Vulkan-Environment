# Settings for external libs

add_subdirectory(volk)

# glfw setup
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" ON)
set(GLFW_BUILD_EXAMPLES FALSE)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" ON)
set(GLFW_BUILD_TESTS FALSE)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" ON)
set(GLFW_BUILD_DOCS FALSE)
option(GLFW_INSTALL "Generate installation target" ON)
set(GLFW_INSTALL FALSE)
include(CMakeDependentOption)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
cmake_dependent_option(USE_MSVC_RUNTIME_LIBRARY_DLL "Use MSVC runtime library DLL" ON
  "MSVC" OFF)
if(VKENV_BUILD_STATIC)
 if(MSVC)
    set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF)
  endif()
  set(BUILD_SHARED_LIBS OFF)
else()
  set(BUILD_SHARED_LIBS ON)
endif()
add_subdirectory(glfw)

set(BUILD_SHARED_LIBS OFF) # dont want shared lib version of glm and volk
add_subdirectory(glm)

# assimp setup
if(NOT NO_ASSIMP)
  OPTION( BUILD_SHARED_LIBS  "Build package with shared libraries." ON)
  if(NOT VKENV_BUILD_STATIC)
    set(BUILD_SHARED_LIBS ON)
  else()
    set(BUILD_SHARED_LIBS OFF)
    OPTION( ASSIMP_BUILD_ZLIB
      "Build your own zlib"
      ON
    )
    set(ASSIMP_BUILD_ZLIB ON)
  endif()
  OPTION ( ASSIMP_WARNINGS_AS_ERRORS
    "Treat all warnings as errors."
    ON
  )
  set(ASSIMP_WARNINGS_AS_ERRORS OFF)
  OPTION( ASSIMP_NO_EXPORT "Disable Assimp's export functionality." OFF)
  set(ASSIMP_NO_EXPORT ON)
  OPTION( ASSIMP_INSTALL "Disable this if you want to use assimp as a submodule." ON )
  set(ASSIMP_INSTALL OFF)
  OPTION ( ASSIMP_BUILD_TESTS "If the test suite for Assimp is built in addition to the library." ON)
  set(ASSIMP_BUILD_TESTS OFF)
  OPTION(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT "default value of all ASSIMP_BUILD_XXX_IMPORTER values" TRUE)
  set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT FALSE)
  # by default only build a few imports
  set(ASSIMP_BUILD_FBX_IMPORTER TRUE)
  set(ASSIMP_BUILD_OBJ_IMPORTER TRUE)
  set(ASSIMP_BUILD_GLTF_IMPORTER TRUE)
  add_subdirectory(assimp)
endif()

if(VKENV_BUILD_STATIC)
  set(BUILD_SHARED_LIBS OFF)
else()
  set(BUILD_SHARED_LIBS ON)
endif()

if(NOT NO_FREETYPE)
  #disable freetype optional dependancies
  option(FT_DISABLE_ZLIB
  "Disable use of system zlib and use internal zlib library instead." OFF)
set(FT_DISABLE_ZLIB true)
option(FT_DISABLE_BZIP2
  "Disable support of bzip2 compressed fonts." OFF)
set(FT_DISABLE_BZIP2 true)
option(FT_DISABLE_PNG
  "Disable support of PNG compressed OpenType embedded bitmaps." OFF)
set(FT_DISABLE_PNG true)
option(FT_DISABLE_HARFBUZZ
  "Disable HarfBuzz (used for improving auto-hinting of OpenType fonts)." OFF)
set(FT_DISABLE_HARFBUZZ true)
option(FT_DISABLE_BROTLI
  "Disable support of compressed WOFF2 fonts." OFF)
  set(FT_DISABLE_BROTLI true)
  add_subdirectory(freetype)
endif()
