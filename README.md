# Instance Rendering Demo
<h3> more up to date 3D renderer on other branch: https://github.com/NoamZeise/Vulkan-Environment</h3>
<h3> working 2D framework on other Branch: https://github.com/NoamZeise/Vulkan-Environment/tree/2D-Environment</h3>



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
