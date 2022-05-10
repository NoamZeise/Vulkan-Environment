cd build
cmake ..
cmake --build . --config Release
cd ..
Copy-Item -Path .\resources\shaders\* -Destination .\build\Release\shaders -Recurse -Force
Copy-Item -Path .\resources\textures\* -Destination .\build\Release\textures -Recurse -Force
Copy-Item -Path .\resources\models\* -Destination .\build\Release\models -Recurse -Force
cd build\Release
./Vulkan-Environment.exe
