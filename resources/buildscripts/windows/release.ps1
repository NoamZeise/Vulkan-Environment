cd build
cmake ..
cmake --build . --config Release
cd ..
Copy-Item -Path .\resources\shaders -Destination .\build\Release -recurse -Force
Copy-Item -Path .\resources\textures -Destination .\build\Release -recurse -Force
Copy-Item -Path .\resources\models -Destination .\build\Release -recurse -Force
cd build\Release
./Vulkan-Environment.exe
