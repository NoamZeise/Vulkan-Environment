for /r %%f in (*.vert) do (
	glslc %%~nf.vert -o v%%~nf.spv
)
for /r %%f in (*.frag) do (
	glslc %%~nf.frag -o f%%~nf.spv
)

cmd