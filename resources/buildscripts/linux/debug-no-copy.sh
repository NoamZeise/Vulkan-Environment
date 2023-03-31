cd build
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=1
cmake --build .
cd demo/Debug
./VKenvDemo
cd ../../../
