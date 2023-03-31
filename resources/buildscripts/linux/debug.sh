cd build
echo "> building app"
cmake .. -DBUILD_DEMO=true
cmake --build . 
cd ..
cd resources/shaders/vulkan
echo "> compiling shaders"
./cmpShader.sh
cd ../../../
echo "> copying files"
cp -r resources/shaders build/demo/Debug
cp -r resources/models build/demo/Debug
cp -r resources/textures build/demo/Debug
cd build/demo/Debug
echo "> running app"
./VKenvDemo
cd ../../../
