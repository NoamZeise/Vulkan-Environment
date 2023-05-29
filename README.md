# Work in progress Vulkan Rendering Framework
A 2D and 3D renderer library that uses Vulkan. In a working state.

## Features:

* 2D and 3D rendering
* Import and Draw models (only supports diffuse image textures)
* Skeletal animation
* Image textures loading and rendering
* Font loading and rendering

## Applications using this library:
* [Robyn Hood](https://github.com/NoamZeise/Robyn-Hood) - 2022 - 2D On-rails Stealth Game
* [Trials of The Pharaoh](https://github.com/NoamZeise/TrailsOfThePharaoh) - 2022 - 2D Light Ray Puzzle Game
* [The Last Dodo](https://github.com/NoamZeise/DodoDash) - 2022 - 2D Platforming Game
* [Get Back Jaxx](https://github.com/NoamZeise/GGJ22) - 2022 -2D Adventure Game
* [Hard Drive Homicide](https://github.com/NoamZeise/Hard-Drive-Homicide) - 2021 - 2D Twin-Stick Bullet Hell Game
* [Battle Island](https://github.com/NoamZeise/Battle-Island) - 2021 - 2D Turn-Based Strategy Game

## External libraries and their uses:

* included: [volk](https://github.com/zeux/volk) dynamically loads pointers to vulkan from driver
* included: [GLFW](https://www.glfw.org/) handles windowing and input
* included: [GLM](https://github.com/g-truc/glm) handles glsl datatypes and linear algebra
* included: [stb_image.h](https://github.com/nothings/stb) handles image loading
* included OPTIONAL:   [Assimp](https://github.com/assimp/assimp) handles model loading
* include OPTIONAL:   [freetype2](https://freetype.org/) handles font loading
* external REQUIRED:   [Vulkan](https://vulkan.lunarg.com/) for vulkan type definitions, used by volk

# Setup

This project uses CMake for building, and can be used as a submodule in your CMake Project if you want to use this library for your application.

There are some optional flags that can be set to change what is built:
- pass `-DBUILD_DEMO=true` to build the example binary
- pass `-DNO_FREETYPE=true` to remove the freetype dependancy from the library (font related functions will throw an exception)
- pass `-DNO_ASSIMP=true` to remove the assimp dependancy from the library ( model related functions will throw an exception)
- pass `-DASSIMP_BUILD_<Some Model Type>_IMPORTER=true` to enable the importing of models that this project doesn't by default. (By default only .fbx and .obj model importers are built)
- pass `-DVKENV_BUILD_STATIC=true` to have the library link agains the system's c runtime statically and link statically to dependancies
- pass `-DBUILD_ASSIMP_STATIC=true` to have assimp linked statically (enabled if `VKENV_BUILD_STATIC` is enabled)

To get this project and all the submodules run `$ git clone <this repo> --recurse-submodules`

GLFW requires the dependancies of your windowing system if you don't have those already, [detailed here](https://www.glfw.org/docs/latest/compile.html#compile_deps).

## Install Dependancies

### Windows

Install the [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/), for getting the vulkan headers and compiling shaders into spirv. make sure the headers can be seen by your compiler.

Install [CMake](https://cmake.org/download/) if you haven't already

### Linux with apt
vulkan tools
```
$ sudo apt install vulkan-tools
```
vulkan loader / validation layers / spriv compilers
```
$ sudo apt install libvulkan-dev vulkan-validationlayers-dev spirv-tools
```
test vulkan works
```
$ vkcube
```
install cmake if you haven't already
```
$ sudo apt install cmake
```
### Building with cmake

make a build folder in the repo dir and go to it
```
mkdir build && cd build
```
setup the build, this is for an example setup for building with the demo in `Release` mode
```
cmake .. -D BUILD_DEMO=true -D CMAKE_BUILD_TYPE=Release
```
build the project
```
cmake --build .
```

You should then have an exectable `VKenvDemo` in `build/demo/` that you can run

## Todo list:
bugs:
* make first-person camera feel normal when fullscreen

features:
* make multiple render passes optional
* make the link between pipelines datatypes and model types more explicit
* customize shader / descriptor sets outside of render
* add tests
* C ABI

optimisations:
* use the same pipeline layout for multiple pipelines
* only buffer static object data to huge ssbo, use smaller one for things that change every frame
* check if animation is faster with array of shader buffers instead
* move visible api to include
* better loading and unloading of textures

