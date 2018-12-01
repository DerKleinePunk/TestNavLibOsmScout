# Test LibOsmScout Functions on Windows with VC2017
Buil on Linux may by possible Cmake ist used

## Build Osmscout with windows an MSVC 2017

You need [vcpkg](https://github.com/Microsoft/vcpkg) 

cd F:\Mine\OpenSource\vcpkg
git pull
bootstrap-vcpkg.bat
cd F:\Mine\OpenSource\libosmscout-code\vcbuild
set path=%PATH%;F:\Mine\OpenSource\vcpkg
vcpkg upgrade --no-dry-run
vcpkg remove --outdated
vcpkg install cairo:x64-windows
vcpkg install sdl2:x64-windows
vcpkg install sdl2-image[libjpeg-turbo]:x64-windows
vcpkg install sdl2-image[libwebp]:x64-windows --recurse
vcpkg install sdl2-mixer:x64-windows
vcpkg install sdl2-net:x64-windows
vcpkg install sdl2-ttf:x64-windows
vcpkg install pango:x64-windows
vcpkg install libxml2:x64-windows
vcpkg install protobuf:x64-windows
vcpkg install glfw3:x64-windows
set QTDIR=F:/Tools/Qt/5.11.2/msvc2017_64
SET MARISA_ROOT=.\marisa
set CMAKE_INCLUDE_PATH=%MARISA_ROOT%\include
set CMAKE_LIBRARY_PATH=%MARISA_ROOT%\lib

cmake -G "Visual Studio 15 2017 Win64" -DQTDIR=%QTDIR% -DCMAKE_PREFIX_PATH=%QTDIR%/lib/cmake -DCMAKE_INSTALL_PREFIX=.\output -DCMAKE_TOOLCHAIN_FILE=F:/Mine/OpenSource/vcpkg/scripts/buildsystems/vcpkg.cmake ..
cmake --build . --target install --config Debug
