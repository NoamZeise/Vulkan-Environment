# Work in progress Vulkan Rendering Framework
A 2D and 3D renderer library that uses Vulkan. In a working state.

## Features:

* 2D and 3D rendering
* Import and Draw models (only supports diffuse image textures)
* Skeletal animation
* Image textures laoding and rendering
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
* external REQUIRED:   [Vulkan](https://vulkan.lunarg.com/) for vulkan type definitions, used by volk
* external OPTIONAL:   [freetype2](https://freetype.org/) handles font loading

# Setup

This project uses CMake for building, and can be used as a submodule in your CMake Project.

There are some optional flags that can be set to change what is built:
- pass `-DBUILD_DEMO=true` to build the example binary
- pass `-DNO_FREETYPE=true` to remove the freetype dependancy from the library (font related functions will throw an exception)
- pass `-DNO_ASSIMP=true` to remove the assimp dependancy from the library ( model related functions will throw an exception)
- pass `-DSTATIC_FLAG=true` to add the `-static` linker option to the compiler (this will only work with GNU compilers)
- pass `-DASSIMP_BUILD_<Some Model Type>_IMPORTER=true` to enable the importing of models that this project doesn't by default. (By default only .fbx and .obj model importers are built)

To get this project and it's submodules run `$ git clone <this repo> --recurse-submodules`

### Windows

GNU compilers are prefered (ie with mingw-w64), untested with microsoft's compiler. You can use something like Msys2 and use `$ pacman -S mingw-w64-x86_64-freetype` to get freetype or [download freetype directly](https://freetype.org/download.html). 

Make sure you have a `FindFreetype.cmake` somewhere [where cmake looks for them](https://cmake.org/cmake/help/latest/command/find_package.html#id6). 

Download the [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/), for getting the vulkan headers and compiling shaders into spirv. make sure the headers can be seen by your compiler.

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
get additional libraries
```
$ sudo apt install libfreetype-dev
```

## Todo list:
bugs:
* make first-person camera feel normal when fullscreen
* working with lavapipe(segfaults atm - something to do with swapchain possibly?)

features:
* make multiple render passes optional

optimisations:
* convert model data to proprietary format with another program to remove assimp dependancy from this project
* use the same pipeline layout for multiple pipelines
* only buffer static object data to huge ssbo, use smaller one for things that change every frame
* more elegant descriptor set + binding handle (too many members of render)
* check if animation is faster with array of shader buffers instead
* resizable descriptor struct
* update all changed DS data in one memcpy might be faster?
* add tests
* move visible api to include
