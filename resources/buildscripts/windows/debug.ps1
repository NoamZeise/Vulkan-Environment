cd build
cmake ..
cmake --build . --config Debug
cd ..
Copy-Item -Path .\resources\shaders -Destination .\build\Debug -recurse -Force
Copy-Item -Path .\resources\textures -Destination .\build\Debug -recurse -Force
Copy-Item -Path .\resources\models -Destination .\build\Debug -recurse -Force
cd build\Debug
./Vulkan-Environment.exe
