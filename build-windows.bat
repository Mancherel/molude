@echo off
echo === Molude Windows Build ===

where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: cmake not found. Install from https://cmake.org/download/
    echo Or: winget install Kitware.CMake
    pause
    exit /b 1
)

if not exist JUCE\CMakeLists.txt (
    echo Initializing JUCE submodule...
    git submodule update --init
)

echo Configuring...
cmake -B build -G "Visual Studio 17 2022" -A x64

echo Building (this may take a few minutes)...
cmake --build build --config Release

echo.
echo === Build complete! ===
echo.
echo VST3 plugin:
echo   build\Molude_artefacts\Release\VST3\Molude.vst3
echo.
echo Standalone app:
echo   build\Molude_artefacts\Release\Standalone\Molude.exe
echo.
echo To install the VST3, copy Molude.vst3 to:
echo   C:\Program Files\Common Files\VST3\
echo.
pause
