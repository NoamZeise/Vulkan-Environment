cd build
cmake .. -G"Ninja Multi-Config" -DCMAKE_EXPORT_COMPILE_COMMANDS=1
cmake --build . --config Debug
cd ..
cp -r resources/audio build/Debug/audio
cp -r resources/shaders build/Debug/shaders
cp -r resources/models build/Debug/models
cp -r resources/textures build/Debug/textures
cd build/Debug
./Vulkan-Environment
cd ../..
