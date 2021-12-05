# Work in progress 3D Vulkan Rendering Framework
<h3> **working 2D framework on '2D-Environment' Branch: https://github.com/NoamZeise/Vulkan-Environment/tree/2D-Environment</h3>

<h5>A 3D renderer for vulkan, ideal for small projects, just add rendering and update code into app.cpp </h5>

features so far:
  
* Handles loading textures and fonts
* Draw textured quads in arbitrary positions
* Draw strings using a loaded font
* Play audio on windows with a filename
* Handle keyboard/mouse input from the user
* Offset texture positions and modify colours


#needs fixing:

*edit image data if gpu does not support less than 4 channels for textures


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
