@echo off
setlocal
set "PACKAGE_DIR=%~1"
if not defined PACKAGE_DIR set "PACKAGE_DIR=dist\QmlCockpit"
cd /d "%~dp0.."
set "PATH=D:\Qt\6.12.0\msvc2022_64\bin;%PATH%"
call "D:\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
if not exist "build\QmlCockpit.exe" exit /b 1
if exist "%PACKAGE_DIR%" (
    echo Choose a new package directory.
    exit /b 1
)
mkdir "%PACKAGE_DIR%" || exit /b 1
copy /y "build\QmlCockpit.exe" "%PACKAGE_DIR%\QmlCockpit.exe" >nul
if errorlevel 1 exit /b 1
for %%F in (sherpa-onnx-c-api.dll onnxruntime.dll onnxruntime_providers_shared.dll) do (
    copy /y "build\%%F" "%PACKAGE_DIR%\%%F" >nul || exit /b 1
)
"D:\Qt\6.12.0\msvc2022_64\bin\windeployqt.exe" --release --no-translations --qmldir qml --dir "%PACKAGE_DIR%" "%PACKAGE_DIR%\QmlCockpit.exe"
if errorlevel 1 exit /b 1
copy /y "README.md" "%PACKAGE_DIR%\README.md" >nul
copy /y "THIRD_PARTY_NOTICES.md" "%PACKAGE_DIR%\THIRD_PARTY_NOTICES.md" >nul
copy /y "scripts\release-qt.conf" "%PACKAGE_DIR%\qt.conf" >nul
if not exist "%PACKAGE_DIR%\licenses" mkdir "%PACKAGE_DIR%\licenses"
copy /y "assets\source\CC-BY-4.0.txt" "%PACKAGE_DIR%\licenses\CarConcept-CC-BY-4.0.txt" >nul
copy /y "assets\source\Khronos-marks.txt" "%PACKAGE_DIR%\licenses\Khronos-marks.txt" >nul
copy /y "assets\sedan\source\MODEL-LICENSE.md" "%PACKAGE_DIR%\licenses\Sedan-MODEL-LICENSE.md" >nul
copy /y "assets\sedan\source\download.json" "%PACKAGE_DIR%\licenses\Sedan-download.json" >nul
copy /y "assets\sedan\source\parts-rig.json" "%PACKAGE_DIR%\licenses\Sedan-parts-rig.json" >nul
copy /y "assets\source\CC-BY-4.0.txt" "%PACKAGE_DIR%\licenses\Sedan-CC-BY-4.0.txt" >nul
xcopy /s /i /y "D:\Qt\Licenses" "%PACKAGE_DIR%\licenses\Qt" >nul || exit /b 1
if not exist "%PACKAGE_DIR%\docs" mkdir "%PACKAGE_DIR%\docs"
copy /y "design\v2\*.md" "%PACKAGE_DIR%\docs\" >nul
if not exist "%PACKAGE_DIR%\docs\states" mkdir "%PACKAGE_DIR%\docs\states"
copy /y "design\v2\states\*.svg" "%PACKAGE_DIR%\docs\states\" >nul
if exist "artifacts\v6\release\27-driving-final.png" copy /y "artifacts\v6\release\27-driving-final.png" "%PACKAGE_DIR%\docs\driving-demo.png" >nul
if exist "artifacts\v6\release\26-status-home.png" copy /y "artifacts\v6\release\26-status-home.png" "%PACKAGE_DIR%\docs\status-home.png" >nul
exit /b 0
