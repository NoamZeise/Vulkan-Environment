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
cp -r resources/shaders build/demo
cp -r resources/models build/demo
cp -r resources/textures build/demo
cd build/demo
echo "> running app"
./VKenvDemo
cd ../..
