for /r %%f in (*.vert) do (
	glslc %%f -o %%f.spv
)
for /r %%f in (*.frag) do (
	glslc %%f -o %%f.spv
)
