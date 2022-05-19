cd build
cmake .. -G"Ninja Multi-Config" -DCMAKE_EXPORT_COMPILE_COMMANDS=1
cmake --build . --config Debug
cd ..
cp -r resources/audio build/Debug
cp -r resources/shaders build/Debug
cp -r resources/models build/Debug
cp -r resources/textures build/Debug
cd build/Debug
./Vulkan-Environment
cd ../..
