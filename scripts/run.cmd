@echo off
setlocal
cd /d "%~dp0.."
set "PATH=D:\Qt\6.12.0\msvc2022_64\bin;%PATH%"
set "COCKPIT_ASSISTANT_MANIFEST=%CD%\runtime\assistant\models.json"
start "" "build\QmlCockpit.exe" %*
