@echo off
setlocal
cd /d "%~dp0.."
set "PATH=D:\Qt\6.12.0\msvc2022_64\bin;%PATH%"
set QT_FORCE_STDERR_LOGGING=1
"D:\Qt\Tools\CMake_64\bin\ctest.exe" --test-dir build --output-on-failure
if errorlevel 1 exit /b 1
"build\QmlCockpit.exe" --smoke-test
exit /b %errorlevel%
