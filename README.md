# Work in progress Vulkan Rendering Framework
A 2D and 3D renderer for vulkan, ideal for small projects, just add rendering and update code into app.cpp. Works with Windows or Linux.

## Features:

* Simultaneous 2D and 3D rendering
* Import and Draw .fbx models -> only supports base colour image textures
* Import and Draw image textures 
* Import and Draw fonts
* Play .wav and .ogg audio files

## Projects using this framework:
* [The Last Dodo](https://github.com/NoamZeise/DodoDash) -> Platforming Game
* [Get Back Jaxx](https://github.com/NoamZeise/GGJ22) -> Adventure Game
* [Hard Drive Homicide](https://github.com/NoamZeise/Hard-Drive-Homicide) -> Twin-Stick Bullet Hell Game
* [Battle Island](https://github.com/NoamZeise/Battle-Island) -> Turn-Based Strategy Game

## Todo list:
bugs:
* make first-person camera feel better

features:
* skeletal animation (and distinguish between animated and non-animated draws)
* make multiple render passes optional

optimisations:
* convert model data to proprietary format with another program to remove assimp dependancy from this project
* use the same pipeline layout for multiple pipelines
* unload old and load new textures while using renderer

## External libraries and their uses:

* [Vulkan lunarG](https://vulkan.lunarg.com/) for vulkan function loader, debugging, validation layers, spriv compilers
* [GLFW](https://www.glfw.org/) handles windowing and input
* [GLM](https://github.com/g-truc/glm) handles glsl datatypes and linear algebra
* [stb_image.h](https://github.com/nothings/stb) handles image loading
* [freetype2](https://freetype.org/) handles font loading
* [Assimp](https://github.com/assimp/assimp) handles model loading
* [libsndfile](https://github.com/libsndfile/libsndfile) handles audio loading
* [portaudio](http://www.portaudio.com/) handles audio playback

# setup



### windows


* download the [vulkan sdk](https://vulkan.lunarg.com/), run installer and put the necessary files in your lib and include directories

* download [glfw3](https://www.glfw.org/), compile and put in your lib and include directories

* download [glm](https://github.com/g-truc/glm), it is header only so put the headers in your include directory

* download [freetype](https://freetype.org/download.html) compile and put in your lib and include directories

* download [assimp](https://github.com/assimp/assimp/blob/master/Build.md) compile and put in your lib and include directories, and distribute the dll with your binaries

* download [libsndfile](http://www.mega-nerd.com/libsndfile/#Download) compile and put in your lib and include directories, and distrubute dll with your binaries

* download [portaudio](http://files.portaudio.com/docs/v19-doxydocs/compile_windows.html) compile and put in your lib and include directories, and distrubute dll with your binaries


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
$ sudo apt-get install libglfw3-dev libglm-dev libfreetype-dev libassimp-dev libsndfile1-dev libasound-dev portaudio19-dev
```
