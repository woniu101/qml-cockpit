@echo off
setlocal
chcp 65001 >nul
set VSLANG=1033
cd /d "%~dp0.."
call "D:\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
"D:\Qt\Tools\CMake_64\bin\cmake.exe" --preset windows-msvc-release
if errorlevel 1 exit /b 1
"D:\Qt\Tools\CMake_64\bin\cmake.exe" --build --preset release %*
exit /b %errorlevel%
