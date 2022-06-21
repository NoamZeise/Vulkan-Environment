cd build
cmake .. -G"Ninja Multi-Config" -DCMAKE_EXPORT_COMPILE_COMMANDS=1
cmake --build . --config Debug
cd Debug
./Vulkan-Environment
cd ../..
