# Work in progress 3D Vulkan Rendering Framework
<h3> working 2D framework on other Branch: https://github.com/NoamZeise/Vulkan-Environment/tree/2D-Environment</h3>

<h5>A 3D renderer for vulkan, ideal for small projects, just add rendering and update code into app.cpp </h5>

# Todo list:

bugs:
* check for supported textures, then convert to supported format if supplied textures are not suitable

features:
* blinn-phong lighting

optimisations:
* create and change pipelines from outside render
* organise draw calls, by storing each call to render.draw then execute draws at render.enddraw  
* use gl_BaseInstance when drawing as an index into an array of model matricies

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
