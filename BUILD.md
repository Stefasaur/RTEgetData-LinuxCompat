# RTEgetData Build Instructions

This document provides comprehensive build instructions for RTEgetData on all supported platforms.

## Quick Start

### Linux/macOS
```bash
./build.sh
```

### Windows (Visual Studio)
```cmd
build.bat
```

### Windows (MinGW)
```cmd
build-mingw.bat
```

## Build Requirements

### All Platforms
- **CMake 3.16 or newer**
- **C++17 compatible compiler**
- **Internet connection** (for downloading dependencies)

### Platform-Specific Requirements

#### Linux
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install cmake build-essential libgl1-mesa-dev

# CentOS/RHEL
sudo yum groupinstall 'Development Tools'
sudo yum install cmake mesa-libGL-devel

# Arch Linux
sudo pacman -S cmake gcc mesa
```

#### macOS
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install CMake (via Homebrew)
brew install cmake
```

#### Windows

**Option 1: Visual Studio (Recommended)**
- Visual Studio 2019 or newer with C++ development tools
- CMake (included with Visual Studio or standalone)

**Option 2: MinGW-w64**
- [MSYS2](https://www.msys2.org/) (recommended)
- [TDM-GCC](https://jmeubank.github.io/tdm-gcc/)
- [WinLibs](https://winlibs.com/)

## Build Options

The build system supports several options:

| Option | Description | Default |
|--------|-------------|---------|
| `BUILD_GUI` | Build GUI version | `ON` |
| `GUI_USE_SDL2` | Use SDL2 instead of GLFW | `ON` |
| `GUI_STATIC_LINK` | Static linking for zero dependencies | `ON` |

## Manual Build (Advanced)

If you need more control over the build process:

### 1. Configure
```bash
# Linux/macOS
mkdir build && cd build
cmake .. -DBUILD_GUI=ON -DGUI_USE_SDL2=ON -DGUI_STATIC_LINK=ON

# Windows (Visual Studio)
mkdir build && cd build
cmake .. -DBUILD_GUI=ON -DGUI_USE_SDL2=ON -DGUI_STATIC_LINK=ON

# Windows (MinGW)
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DBUILD_GUI=ON -DGUI_USE_SDL2=ON -DGUI_STATIC_LINK=ON
```

### 2. Build
```bash
# Linux/macOS
make -j$(nproc)

# Windows (Visual Studio)
cmake --build . --config Release

# Windows (MinGW)
mingw32-make -j%NUMBER_OF_PROCESSORS%
```

## Output Files

After successful build:

### Linux/macOS
- **CLI**: `build/RTEgetData` (~59KB)
- **GUI**: `build/RTEgetData-GUI` (~5.3MB)

### Windows
- **CLI**: `build/Release/RTEgetData.exe` or `build/RTEgetData.exe` (~60KB)
- **GUI**: `build/Release/RTEgetData-GUI.exe` or `build/RTEgetData-GUI.exe` (~5.5MB)

## Troubleshooting

### Build Configuration Fails

**"No suitable backend found"**
```bash
# Try GLFW backend instead
cmake .. -DBUILD_GUI=ON -DGUI_USE_SDL2=OFF -DGUI_STATIC_LINK=ON
```

**CMake version too old**
- Install CMake 3.16+ from [cmake.org](https://cmake.org/download/)

**Compiler not found**
- Linux: Install build tools (`build-essential`, `gcc`, etc.)
- Windows: Install Visual Studio with C++ tools or MinGW-w64

### Build Process Fails

**Internet connection required**
- The build downloads ImGui, SDL2/GLFW automatically
- Ensure stable internet connection during first build

**Insufficient memory**
- Static linking requires significant memory
- Close other applications or add swap space
- Use fewer parallel jobs: `make -j2` instead of `make -j$(nproc)`

**Missing OpenGL**
- Linux: Install Mesa development packages
- Windows: Usually pre-installed with graphics drivers

### Runtime Issues

**GUI doesn't start**
- Install/update graphics drivers
- Verify OpenGL support: `glxinfo | grep OpenGL` (Linux)

**COM port access denied (Linux)**
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER
# Log out and log back in
```

## Dependencies

The build system automatically downloads and statically links:

- **Dear ImGui** 1.90.9 - GUI framework
- **SDL2** 2.28.5 - Window/input management (default)
- **GLFW** 3.3.8 - Alternative window management
- **OpenGL** - Hardware-accelerated rendering (system provided)

## Cross-Platform Notes

### Static Linking
All builds use static linking by default, producing self-contained executables:
- **No runtime dependencies** (except system OpenGL)
- **Easy distribution** - just copy the executable
- **Larger file sizes** - but typically under 6MB for GUI

### Performance
- CLI version: Minimal overhead, ~59-60KB
- GUI version: Modern OpenGL rendering, ~5.3-5.5MB
- Both versions support the same RTEgetData functionality

### Compatibility
- **Linux**: Works on most distributions with OpenGL support
- **Windows**: Windows 7+ with OpenGL drivers
- **macOS**: macOS 10.12+ with OpenGL/Metal support

## Getting Help

If you encounter build issues:

1. Check this troubleshooting section
2. Ensure all requirements are installed
3. Try the fallback build options
4. Report issues with full build logs

## Examples

### Basic Usage After Build
```bash
# CLI version - connect via GDB server
./RTEgetData 3333 0x24000000 0x2000

# GUI version - interactive interface
./RTEgetData-GUI
```

### COM Port Examples
```bash
# Linux
./RTEgetData /dev/ttyUSB0 0x24000000 0x2000

# Windows  
RTEgetData.exe COM1 0x24000000 0x2000
```

The executables are fully portable and can be distributed to other machines of the same architecture without additional setup.