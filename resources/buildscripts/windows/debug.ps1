cd build
cmake ..
cmake --build . --config Debug
cd ..
Copy-Item -Path .\resources\shaders\* -Destination .\build\Debug\shaders -Recurse -Force
Copy-Item -Path .\resources\textures\* -Destination .\build\Debug\textures -Recurse -Force
Copy-Item -Path .\resources\models\* -Destination .\build\Debug\models -Recurse -Force
cd build\Debug
./Vulkan-Environment.exe
