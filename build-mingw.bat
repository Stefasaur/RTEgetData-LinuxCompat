@echo off
REM RTEgetData Build Script for Windows with MinGW
REM Alternative to MSVC build using MinGW-w64 compiler

echo ==================================================
echo RTEgetData Windows Build Script ^(MinGW^)
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

REM Check if MinGW is available
gcc --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: MinGW-w64 GCC compiler not found
    echo Please install MinGW-w64:
    echo   Option 1: MSYS2 ^(recommended^) - https://www.msys2.org/
    echo   Option 2: TDM-GCC - https://jmeubank.github.io/tdm-gcc/
    echo   Option 3: WinLibs - https://winlibs.com/
    echo.
    echo Make sure GCC is in your PATH environment variable.
    pause
    exit /b 1
)

REM Check for build directory
if not exist "build" (
    echo Creating build directory...
    mkdir build
)

cd build

REM Configure CMake with MinGW Makefiles and static linking
echo Configuring CMake build with MinGW...
echo - Building both CLI and GUI versions
echo - Using SDL2 backend with static linking
echo - Creating self-contained executables
echo.

cmake .. -G "MinGW Makefiles" -DBUILD_GUI=ON -DGUI_USE_SDL2=ON -DGUI_STATIC_LINK=ON

if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed!
    echo.
    echo Common solutions:
    echo 1. Ensure MinGW-w64 is properly installed and in PATH
    echo 2. Check internet connection ^(downloads dependencies^)
    echo 3. Try fallback: cmake .. -G "MinGW Makefiles" -DBUILD_GUI=ON -DGUI_USE_SDL2=OFF -DGUI_STATIC_LINK=ON
    echo.
    pause
    exit /b 1
)

REM Build the project with parallel jobs
echo.
echo Building RTEgetData with MinGW...
mingw32-make -j%NUMBER_OF_PROCESSORS%

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
if exist "RTEgetData.exe" (
    echo   CLI Version: build\RTEgetData.exe
)

if exist "RTEgetData-GUI.exe" (
    echo   GUI Version: build\RTEgetData-GUI.exe
)

echo.
echo Usage Examples:
echo   CLI: RTEgetData.exe 3333 0x24000000 0x2000
echo   GUI: RTEgetData-GUI.exe
echo.
echo File sizes:
if exist "RTEgetData.exe" (
    for %%I in ("RTEgetData.exe") do echo   CLI: %%~zI bytes
)

if exist "RTEgetData-GUI.exe" (
    for %%I in ("RTEgetData-GUI.exe") do echo   GUI: %%~zI bytes
)

echo.
echo The executables are fully self-contained and can be
echo distributed without additional dependencies.
echo.
echo Compiler Used: MinGW-w64 GCC
gcc --version | findstr gcc
echo.
pause