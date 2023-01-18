# Work in progress Vulkan Rendering Framework
A 2D and 3D renderer for vulkan, ideal for small projects, just add rendering and update code into app.cpp. Works with Windows or Linux.

## Features:

* Simultaneous 2D and 3D rendering
* Import and Draw .fbx models -> only supports diffuse image textures
* Skeletal animation of .fbx models
* Import and Draw image textures
* Import and Draw fonts

## Projects using this framework:
* [Robyn Hood](https://github.com/NoamZeise/Robyn-Hood) -> 2D On-rails Stealth Game
* [Trials of The Pharaoh](https://github.com/NoamZeise/TrailsOfThePharaoh) -> 2D Light Ray Puzzle Game
* [The Last Dodo](https://github.com/NoamZeise/DodoDash) -> 2D Platforming Game
* [Get Back Jaxx](https://github.com/NoamZeise/GGJ22) -> 2D Adventure Game
* [Hard Drive Homicide](https://github.com/NoamZeise/Hard-Drive-Homicide) -> 2D Twin-Stick Bullet Hell Game
* [Battle Island](https://github.com/NoamZeise/Battle-Island) -> 2D Turn-Based Strategy Game

## Todo list:
bugs:
* make first-person camera feel normal when fullscreen
* working with lavapipe(segfaults atm - something to do with swapchain possibly?)

features:
* make multiple render passes optional

optimisations:
* convert model data to proprietary format with another program to remove assimp dependancy from this project
* use the same pipeline layout for multiple pipelines
* unload old and load new textures while using renderer
* only buffer static object data to huge ssbo, use smaller one for things that change every frame
* more elegant descriptor set + binding handle (too many members of render)
* check if animation is faster with array of shader buffers instead
* resizable descriptor struct
* update all changed DS data in one memcpy might be faster?

## External libraries and their uses:

* [volk](https://github.com/zeux/volk) dynamically loads pointers to vulkan functions from driver  
* [GLFW](https://www.glfw.org/) handles windowing and input
* [GLM](https://github.com/g-truc/glm) handles glsl datatypes and linear algebra
* [stb_image.h](https://github.com/nothings/stb) handles image loading
* [freetype2](https://freetype.org/) handles font loading
* [Assimp](https://github.com/assimp/assimp) handles model loading
* [libsndfile](https://github.com/libsndfile/libsndfile) handles audio loading
* [portaudio](http://www.portaudio.com/) handles audio playback

# setup

### windows

* ensure your graphics drivers support vulkan 

* download [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) for compiling shaders into spirv

* download [glfw3](https://www.glfw.org/), compile and put in your lib and include directories

* download [glm](https://github.com/g-truc/glm), it is header only so put the headers in your include directory

* download [freetype](https://freetype.org/download.html) compile and put in your lib and include directories

* download [assimp](https://github.com/assimp/assimp/blob/master/Build.md) compile and put in your lib and include directories, and distribute the dll with your binaries

* set your lib and include paths at the start of the cmake file
```
#windows only
set(Lib "Path/to/lib")
set(Include "Path/to/include")
```

* If you are using the msvc compiler, you can use the included windows build scripts under "resources/buildscripts/windows/" to build the debug or release versions of the project and automatically launch it. You must include the assimp .dll with the project.

### linux with apt
vulkan tools
```
$ sudo apt-get install vulkan-tools
```
vulkan loader / validation layers / spriv compilers
```
$ sudo apt-get install libvulkan-dev vulkan-validationlayers-dev spirv-tools
```
test vulkan works
```
$ vkcube
```
additional libraries
```
$ sudo apt-get install libglfw3-dev libglm-dev libfreetype-dev libassimp-dev
```
