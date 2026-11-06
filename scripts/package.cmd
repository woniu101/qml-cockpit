@echo off
setlocal
cd /d "%~dp0.."
set "PATH=D:\Qt\6.12.0\msvc2022_64\bin;%PATH%"
call "D:\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
if not exist "build\QmlCockpit.exe" exit /b 1
if not exist "dist\QmlCockpit" mkdir "dist\QmlCockpit"
copy /y "build\QmlCockpit.exe" "dist\QmlCockpit\QmlCockpit.exe" >nul
if errorlevel 1 exit /b 1
"D:\Qt\6.12.0\msvc2022_64\bin\windeployqt.exe" --release --no-translations --qmldir qml --dir "dist\QmlCockpit" "dist\QmlCockpit\QmlCockpit.exe"
if errorlevel 1 exit /b 1
copy /y "README.md" "dist\QmlCockpit\README.md" >nul
copy /y "THIRD_PARTY_NOTICES.md" "dist\QmlCockpit\THIRD_PARTY_NOTICES.md" >nul
copy /y "scripts\release-qt.conf" "dist\QmlCockpit\qt.conf" >nul
if not exist "dist\QmlCockpit\licenses" mkdir "dist\QmlCockpit\licenses"
copy /y "assets\source\CC-BY-4.0.txt" "dist\QmlCockpit\licenses\CarConcept-CC-BY-4.0.txt" >nul
copy /y "assets\source\Khronos-marks.txt" "dist\QmlCockpit\licenses\Khronos-marks.txt" >nul
copy /y "assets\sedan\source\MODEL-LICENSE.md" "dist\QmlCockpit\licenses\Sedan-MODEL-LICENSE.md" >nul
copy /y "assets\sedan\source\download.json" "dist\QmlCockpit\licenses\Sedan-download.json" >nul
copy /y "assets\sedan\source\parts-rig.json" "dist\QmlCockpit\licenses\Sedan-parts-rig.json" >nul
copy /y "assets\source\CC-BY-4.0.txt" "dist\QmlCockpit\licenses\Sedan-CC-BY-4.0.txt" >nul
xcopy /s /i /y "D:\Qt\Licenses" "dist\QmlCockpit\licenses\Qt" >nul
if not exist "dist\QmlCockpit\docs" mkdir "dist\QmlCockpit\docs"
copy /y "design\v2\*.md" "dist\QmlCockpit\docs\" >nul
if not exist "dist\QmlCockpit\docs\states" mkdir "dist\QmlCockpit\docs\states"
copy /y "design\v2\states\*.svg" "dist\QmlCockpit\docs\states\" >nul
if exist "artifacts\v6\release\27-driving-final.png" copy /y "artifacts\v6\release\27-driving-final.png" "dist\QmlCockpit\docs\driving-demo.png" >nul
if exist "artifacts\v6\release\26-status-home.png" copy /y "artifacts\v6\release\26-status-home.png" "dist\QmlCockpit\docs\status-home.png" >nul
exit /b 0
