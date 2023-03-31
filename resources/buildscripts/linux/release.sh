cd build
cmake .. -G"Ninja Multi-Config" -DCMAKE_EXPORT_COMPILE_COMMANDS=1
cmake --build . --config Release
cd ..
cp -r resources/shaders build/demo/Release
cp -r resources/models build/demo/Release
cp -r resources/textures build/demo/Release
cd build/demo/Release
./VKenvDemo
cd ../../../
