call "C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
cd build
cmake ..
msbuild ALL_BUILD.vcxproj



cd ..
cd remote-ui/build
C:\Qt\6.2.1\mingw81_64\bin\qmake.exe ../remote-ui.pro
C:\Qt\Tools\mingw810_64\bin\mingw32-make.exe
