# Work in progress 3D Vulkan Rendering Framework
<h5>A 2D and 3D renderer for vulkan, ideal for small projects, just add rendering and update code into app.cpp </h5>

# Features:

* Simultaneous 2D and 3D rendering
* Import and Draw .fbx models
* Import and Draw RGBA textures 
* Import and Draw fonts
* Instance rendering for fast drawing of the same objects in multiple positions
* Blinn-Phong for 3D models

# Projects using this framework:
* https://github.com/NoamZeise/GGJ22
* https://github.com/NoamZeise/Hard-Drive-Homicide
* https://github.com/NoamZeise/Battle-Island

# Todo list:
bugs:
* check for supported textures, then convert to supported format if supplied textures are not suitable
* fix scroll wheel input on very short updates
* 
features:
* skeletal animation (and distinguish between animated and non-animated draws)
* 
optimisations:
* create and change pipelines from outside render class?

# dependancies:

* vulkan sdk (i'm using 1.2.189.1) for included libs, validation layers when debugging, glsl to spirv
* GLFW handles windowing and input
* (on UNIX) only setup for X11 with glfw
* GLM handles glsl datatypes and matrix operations
* freetype2 for font loading (comes with repo)
* uses stb_image.h for image loading (comes with repo)

libs (windows only):
* freetyped.lib (included)
* vulkan-1.lib (included)
* glfw3.lib (included)
* Winmm.lib (if on windows, for audio)
