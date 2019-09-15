mkdir build
mkdir build\msvc2019
cd build\msvc2019
cmake -G "Visual Studio 16 2019" ..\..
cmake --build . -j --target allocator
cd ..\..
