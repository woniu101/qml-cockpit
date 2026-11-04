@echo off
setlocal
set VSLANG=1033
cd /d "%~dp0.."
call "D:\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
"D:\Qt\Tools\CMake_64\bin\cmake.exe" -S tools/media -B build/media-tools -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM=D:/Qt/Tools/Ninja/ninja.exe -DCMAKE_PREFIX_PATH=D:/Qt/6.12.0/msvc2022_64
if errorlevel 1 exit /b 1
"D:\Qt\Tools\CMake_64\bin\cmake.exe" --build build/media-tools
exit /b %errorlevel%
