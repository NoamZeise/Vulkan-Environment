# Work in progress Vulkan Rendering Framework
A 2D and 3D renderer library that uses Vulkan

## Features:

* 2D and 3D rendering
* Import and Draw models (only supports diffuse image textures)
* Skeletal animation
* Image textures laoding and rendering
* Font loading and rendering

## Projects using this framework:
* [Robyn Hood](https://github.com/NoamZeise/Robyn-Hood) - 2D On-rails Stealth Game
* [Trials of The Pharaoh](https://github.com/NoamZeise/TrailsOfThePharaoh) - 2D Light Ray Puzzle Game
* [The Last Dodo](https://github.com/NoamZeise/DodoDash) - 2D Platforming Game
* [Get Back Jaxx](https://github.com/NoamZeise/GGJ22) - 2D Adventure Game
* [Hard Drive Homicide](https://github.com/NoamZeise/Hard-Drive-Homicide) - 2D Twin-Stick Bullet Hell Game
* [Battle Island](https://github.com/NoamZeise/Battle-Island) - 2D Turn-Based Strategy Game

## External libraries and their uses:

* included in project: [volk](https://github.com/zeux/volk) dynamically loads pointers to vulkan from driver
* included in project: [GLFW](https://www.glfw.org/) handles windowing and input
* included in project: [GLM](https://github.com/g-truc/glm) handles glsl datatypes and linear algebra
* included in project: [stb_image.h](https://github.com/nothings/stb) handles image loading
* external REQUIRED:   [Vulkan](https://vulkan.lunarg.com/) for vulkan type definitions, used by volk
* external OPTIONAL:   [freetype2](https://freetype.org/) handles font loading
* external OPTIONAL:   [Assimp](https://github.com/assimp/assimp) handles model loading

# Setup

This project uses CMake for building, and can be used as a submodule in your CMake Project.

There are some optional flags that can be set to change the build:
- pass `-DBUILD_DEMO=true` to build the example binary
- pass `-DNO_FREETYPE=true` to remove the freetype dependancy from the library (font related functions will throw an exception)
- pass `-DNO_ASSIMP=true` to remove the assimp dependancy from the library ( model related functions will throw an exception)
- pass `-DSTATIC_FLAG=true` to add the `-static` linker option to the compiler (this will only work with GNU compilers)

To get this project and it's submodules run `$ git clone <this repo> --recurse-submodules`

### Windows

GNU compilers are prefered (ie with mingw-w64), untested with microsoft's compiler.

* ensure your graphics drivers support vulkan 

* download [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) for compiling shaders into spirv

* download [freetype](https://freetype.org/download.html) compile and put in your lib and include directories, and have a `Find<Packagename>.cmake` script in a place that cmake can see, distribute freetype dlls with your binaries

* download [assimp](https://github.com/assimp/assimp/blob/master/Build.md) compile and put in your lib and include directories, and have a `Find<Packagename>.cmake` script in a place that cmake can see, and distribute the dll with your binaries


Note: The `Find<Packagename>.cmake` scripts are looked for by cmake according to [these rules](https://cmake.org/cmake/help/latest/command/find_package.html#id6)


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
$ sudo apt install libfreetype-dev libassimp-dev
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
* move visible api to include
