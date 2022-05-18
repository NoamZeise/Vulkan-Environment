cd build
cmake .. -G"Ninja Multi-Config" -DCMAKE_EXPORT_COMPILE_COMMANDS=1
cmake --build . --config Release
cd ..
cp -r resources/audio build/Release/audio
cp -r resources/shaders build/Release/shaders
cp -r resources/models build/Release/models
cp -r resources/textures build/Release/textures
cd build/Release
./Vulkan-Environment
cd ../..
