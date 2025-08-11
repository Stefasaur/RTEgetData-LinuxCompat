@echo off
REM RTEgetData Build Script for Windows
REM Builds both CLI and GUI versions with static linking

echo ==================================================
echo RTEgetData Windows Build Script
echo ==================================================
echo.

REM Check if CMake is available
cmake --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake is not installed or not in PATH
    echo Please install CMake 3.16 or newer from https://cmake.org/download/
    pause
    exit /b 1
)

REM Check for build directory
if not exist "build" (
    echo Creating build directory...
    mkdir build
)

cd build

REM Configure CMake with static linking for both SDL2 and GUI
echo Configuring CMake build...
echo - Building both CLI and GUI versions
echo - Using SDL2 backend with static linking
echo - Creating self-contained executables
echo.

cmake .. -DBUILD_GUI=ON -DGUI_USE_SDL2=ON -DGUI_STATIC_LINK=ON

if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed!
    echo.
    echo Common solutions:
    echo 1. Ensure you have a C++17 compatible compiler
    echo 2. Check internet connection ^(downloads dependencies^)
    echo 3. Try fallback: cmake .. -DBUILD_GUI=ON -DGUI_USE_SDL2=OFF -DGUI_STATIC_LINK=ON
    echo.
    pause
    exit /b 1
)

REM Build the project
echo.
echo Building RTEgetData...
cmake --build . --config Release

if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    echo Check the error messages above for details.
    pause
    exit /b 1
)

echo.
echo ==================================================
echo BUILD SUCCESSFUL!
echo ==================================================
echo.
echo Built executables:
if exist "Release\RTEgetData.exe" (
    echo   CLI Version: build\Release\RTEgetData.exe
) else if exist "RTEgetData.exe" (
    echo   CLI Version: build\RTEgetData.exe
)

if exist "Release\RTEgetData-GUI.exe" (
    echo   GUI Version: build\Release\RTEgetData-GUI.exe
) else if exist "RTEgetData-GUI.exe" (
    echo   GUI Version: build\RTEgetData-GUI.exe
)

echo.
echo Usage Examples:
echo   CLI: RTEgetData.exe 3333 0x24000000 0x2000
echo   GUI: RTEgetData-GUI.exe
echo.
echo File sizes:
if exist "Release\RTEgetData.exe" (
    for %%I in ("Release\RTEgetData.exe") do echo   CLI: %%~zI bytes
) else if exist "RTEgetData.exe" (
    for %%I in ("RTEgetData.exe") do echo   CLI: %%~zI bytes
)

if exist "Release\RTEgetData-GUI.exe" (
    for %%I in ("Release\RTEgetData-GUI.exe") do echo   GUI: %%~zI bytes
) else if exist "RTEgetData-GUI.exe" (
    for %%I in ("RTEgetData-GUI.exe") do echo   GUI: %%~zI bytes
)

echo.
echo The executables are fully self-contained and can be
echo distributed without additional dependencies.
echo.
pause