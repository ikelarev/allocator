mkdir build
mkdir build\msvc2017
cd build\msvc2017
cmake -G "Visual Studio 15 2017 Win64" ..\..
cmake --build . -j --target allocator
cd ..\..
