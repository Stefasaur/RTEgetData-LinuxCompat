#!/bin/bash
# RTEgetData Build Script for Linux/macOS
# Builds both CLI and GUI versions with static linking

set -e  # Exit on any error

echo "=================================================="
echo "RTEgetData Linux/macOS Build Script"
echo "=================================================="
echo

# Check if CMake is available
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake is not installed"
    echo "Please install CMake 3.16 or newer:"
    echo "  Ubuntu/Debian: sudo apt install cmake"
    echo "  CentOS/RHEL:   sudo yum install cmake"
    echo "  macOS:         brew install cmake"
    exit 1
fi

# Check CMake version
CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
CMAKE_MAJOR=$(echo $CMAKE_VERSION | cut -d'.' -f1)
CMAKE_MINOR=$(echo $CMAKE_VERSION | cut -d'.' -f2)

if [ "$CMAKE_MAJOR" -lt 3 ] || ([ "$CMAKE_MAJOR" -eq 3 ] && [ "$CMAKE_MINOR" -lt 16 ]); then
    echo "ERROR: CMake version $CMAKE_VERSION is too old"
    echo "Please install CMake 3.16 or newer"
    exit 1
fi

# Check for C++ compiler
if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
    echo "ERROR: No C++ compiler found (g++ or clang++)"
    echo "Please install a C++17 compatible compiler:"
    echo "  Ubuntu/Debian: sudo apt install build-essential"
    echo "  CentOS/RHEL:   sudo yum groupinstall 'Development Tools'"
    echo "  macOS:         xcode-select --install"
    exit 1
fi

# Detect number of CPU cores for parallel build
if command -v nproc &> /dev/null; then
    CORES=$(nproc)
elif [ -f /proc/cpuinfo ]; then
    CORES=$(grep -c ^processor /proc/cpuinfo)
else
    CORES=4  # Default fallback
fi

echo "Detected $CORES CPU cores for parallel build"

# Create build directory
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

# Configure CMake with static linking
echo "Configuring CMake build..."
echo "- Building both CLI and GUI versions"
echo "- Using SDL2 backend with static linking" 
echo "- Creating self-contained executables"
echo

if ! cmake .. -DBUILD_GUI=ON -DGUI_USE_SDL2=ON -DGUI_STATIC_LINK=ON; then
    echo
    echo "ERROR: CMake configuration failed!"
    echo
    echo "Common solutions:"
    echo "1. Install required packages:"
    echo "   Ubuntu/Debian: sudo apt install libgl1-mesa-dev"
    echo "2. Check internet connection (downloads dependencies)"
    echo "3. Try fallback: cmake .. -DBUILD_GUI=ON -DGUI_USE_SDL2=OFF -DGUI_STATIC_LINK=ON"
    echo
    exit 1
fi

# Build the project
echo
echo "Building RTEgetData with $CORES parallel jobs..."
if ! make -j$CORES; then
    echo
    echo "ERROR: Build failed!"
    echo "Check the error messages above for details."
    exit 1
fi

echo
echo "=================================================="
echo "BUILD SUCCESSFUL!"
echo "=================================================="
echo
echo "Built executables:"

if [ -f "RTEgetData" ]; then
    echo "  CLI Version: build/RTEgetData"
fi

if [ -f "RTEgetData-GUI" ]; then
    echo "  GUI Version: build/RTEgetData-GUI"
fi

echo
echo "Usage Examples:"
echo "  CLI: ./RTEgetData 3333 0x24000000 0x2000"
echo "  GUI: ./RTEgetData-GUI"

echo
echo "File sizes:"
if [ -f "RTEgetData" ]; then
    echo -n "  CLI: "
    ls -lh RTEgetData | awk '{print $5}'
fi

if [ -f "RTEgetData-GUI" ]; then
    echo -n "  GUI: "
    ls -lh RTEgetData-GUI | awk '{print $5}'
fi

echo
echo "The executables are fully self-contained and can be"
echo "distributed without additional dependencies."

# Check for COM port access permissions on Linux
if [ "$(uname)" = "Linux" ]; then
    echo
    echo "NOTE: For COM port access on Linux, add your user to the dialout group:"
    echo "  sudo usermod -a -G dialout \$USER"
    echo "  (then log out and log back in)"
fi

echo