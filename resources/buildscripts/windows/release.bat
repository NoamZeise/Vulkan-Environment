SETLOCAL
cd build
call vcvars64
cmake .. -G"Ninja Multi-Config" -DCMAKE_EXPORT_COMPILE_COMMANDS=1
cmake --build . --config Release
cd ..
Xcopy resources\models build\Release\models /i /c /e /r /y
Xcopy resources\shaders build\Release\shaders /i /c /e /r /y
Xcopy resources\textures build\Release\textures /i /c /e /r /y
Xcopy resources\windows-dlls build\Release\ /i /c /e /r /y
cd build\Release
Vulkan-Environment
ENDLOCAL
