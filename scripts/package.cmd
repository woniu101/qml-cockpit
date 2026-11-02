@echo off
setlocal
cd /d "%~dp0.."
set "PATH=D:\Qt\6.12.0\msvc2022_64\bin;%PATH%"
call "D:\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
if not exist "build\CarTerminal.exe" exit /b 1
if not exist "dist\CarTerminal" mkdir "dist\CarTerminal"
copy /y "build\CarTerminal.exe" "dist\CarTerminal\CarTerminal.exe" >nul
if errorlevel 1 exit /b 1
"D:\Qt\6.12.0\msvc2022_64\bin\windeployqt.exe" --release --no-translations --qmldir qml --dir "dist\CarTerminal" "dist\CarTerminal\CarTerminal.exe"
if errorlevel 1 exit /b 1
copy /y "README.md" "dist\CarTerminal\README.md" >nul
copy /y "THIRD_PARTY_NOTICES.md" "dist\CarTerminal\THIRD_PARTY_NOTICES.md" >nul
copy /y "scripts\release-qt.conf" "dist\CarTerminal\qt.conf" >nul
if not exist "dist\CarTerminal\licenses" mkdir "dist\CarTerminal\licenses"
copy /y "assets\source\CC-BY-4.0.txt" "dist\CarTerminal\licenses\CarConcept-CC-BY-4.0.txt" >nul
copy /y "assets\source\Khronos-marks.txt" "dist\CarTerminal\licenses\Khronos-marks.txt" >nul
copy /y "assets\sedan\source\MODEL-LICENSE.md" "dist\CarTerminal\licenses\Sedan-MODEL-LICENSE.md" >nul
copy /y "assets\sedan\source\download.json" "dist\CarTerminal\licenses\Sedan-download.json" >nul
copy /y "assets\sedan\source\parts-rig.json" "dist\CarTerminal\licenses\Sedan-parts-rig.json" >nul
copy /y "assets\source\CC-BY-4.0.txt" "dist\CarTerminal\licenses\Sedan-CC-BY-4.0.txt" >nul
xcopy /s /i /y "D:\Qt\Licenses" "dist\CarTerminal\licenses\Qt" >nul
if not exist "dist\CarTerminal\docs" mkdir "dist\CarTerminal\docs"
copy /y "design\v2\*.md" "dist\CarTerminal\docs\" >nul
if not exist "dist\CarTerminal\docs\states" mkdir "dist\CarTerminal\docs\states"
copy /y "design\v2\states\*.svg" "dist\CarTerminal\docs\states\" >nul
if exist "artifacts\v6\release\27-driving-final.png" copy /y "artifacts\v6\release\27-driving-final.png" "dist\CarTerminal\docs\driving-demo.png" >nul
if exist "artifacts\v6\release\26-status-home.png" copy /y "artifacts\v6\release\26-status-home.png" "dist\CarTerminal\docs\status-home.png" >nul
exit /b 0
