cd build
cmake .. -G"Ninja Multi-Config" -DCMAKE_EXPORT_COMPILE_COMMANDS=1
cmake --build . --config Release
cd ..
cp -r resources/audio build/Release
cp -r resources/shaders build/Release
cp -r resources/models build/Release
cp -r resources/textures build/Release
cd build/Release
./Vulkan-Environment
cd ../..
