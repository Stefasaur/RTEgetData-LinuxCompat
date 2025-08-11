# RTEgetData GUI

A cross-platform graphical user interface for RTEgetData built with Dear ImGui.

## Features

- **Modern GUI Interface**: Intuitive point-and-click interface with real-time progress tracking
- **Cross-Platform**: Works on Windows, Linux, and macOS
- **Zero Dependencies**: Fully self-contained executable - no GUI libraries to install
- **Dual Connection Support**: Both GDB server (TCP/IP) and COM port connections
- **Real-Time Logging**: Live message display with timestamps and color-coded severity levels
- **Background Operations**: Non-blocking data transfers with progress visualization
- **Settings Persistence**: Automatically saves and restores connection settings

## Quick Start

### Download & Run

**Linux:**
```bash
# Download or build RTEgetData-GUI
./RTEgetData-GUI
```

**Windows:**
```cmd
# Download or build RTEgetData-GUI.exe
RTEgetData-GUI.exe
```

That's it! No additional software installation required.

## Building from Source

### Prerequisites

- CMake 3.16 or newer
- C++17 compatible compiler (GCC, Clang, MSVC)
- Git (for downloading dependencies)

### Build Instructions

**Linux/macOS:**
```bash
git clone <repository-url>
cd RTEgetData
mkdir build && cd build

# Build with GUI (default: SDL2 backend, static linking)
cmake .. -DGUI_USE_SDL2=ON -DGUI_STATIC_LINK=ON
make -j$(nproc)

# Alternative: GLFW backend
cmake .. -DGUI_USE_SDL2=OFF -DGUI_STATIC_LINK=ON
make -j$(nproc)
```

**Windows (Visual Studio):**
```cmd
git clone <repository-url>
cd RTEgetData
mkdir build && cd build

cmake .. -DGUI_USE_SDL2=ON -DGUI_STATIC_LINK=ON
cmake --build . --config Release
```

**Windows (MinGW):**
```cmd
cmake .. -G "MinGW Makefiles" -DGUI_USE_SDL2=ON -DGUI_STATIC_LINK=ON
mingw32-make -j%NUMBER_OF_PROCESSORS%
```

### Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `BUILD_GUI` | Enable GUI build | `ON` |
| `GUI_USE_SDL2` | Use SDL2 backend instead of GLFW | `ON` |
| `GUI_STATIC_LINK` | Statically link all dependencies | `ON` |

## Using the GUI

### Main Interface

1. **Connection Type**: Choose between GDB Server or COM Port
2. **Connection Settings**: 
   - **GDB Server**: IP address and port
   - **COM Port**: Port name, baud rate, parity, stop bits, timeout
3. **Transfer Settings**: Memory address, size, output file
4. **Action Buttons**: Connect & Transfer, Clear Log

### GDB Server Connection

- **IP Address**: Target GDB server IP (default: 127.0.0.1)
- **Port**: GDB server port (default: 3333)

### COM Port Connection

- **Port**: Serial port device
  - Linux: `/dev/ttyUSB0`, `/dev/ttyACM0`, `/dev/ttyS0`
  - Windows: `COM1`, `COM2`, etc.
- **Baud Rate**: Communication speed (default: 115200)
- **Parity**: None, Odd, Even
- **Stop Bits**: 1 or 2
- **Timeout**: Response timeout in milliseconds
- **Single Wire Mode**: Enable for single-wire communication

### Data Transfer

1. Set **Memory Address** (hex format, e.g., `0x24000000`)
2. Set **Size** (hex format, e.g., `0x2000`)
3. Choose **Output File** for binary data
4. Click **Connect & Transfer**

### Log Window

- **Auto-scroll**: Automatically scroll to newest messages
- **Show timestamps**: Display time for each log entry
- **Color coding**:
  - White: Information
  - Yellow: Warnings  
  - Red: Errors
  - Gray: Debug messages

## Architecture

### Backend Support

The GUI supports two rendering backends:

- **SDL2** (default): Excellent cross-platform support, smaller binary
- **GLFW**: Alternative backend, good OpenGL integration

Both backends are automatically downloaded and statically linked during build.

### Static Linking

All GUI dependencies are statically linked:
- SDL2/GLFW: Window management and input
- Dear ImGui: Immediate mode GUI framework
- OpenGL: Hardware-accelerated rendering

This results in:
- **Single executable distribution**
- **No runtime dependencies** (except system OpenGL)
- **Easy deployment** across different systems

### Cross-Platform Compatibility

| Platform | CLI Size | GUI Size | System Requirements |
|----------|----------|----------|-------------------|
| Linux    | ~59KB    | ~5.4MB   | OpenGL drivers |
| Windows  | ~60KB    | ~5.5MB   | OpenGL drivers |
| macOS    | ~60KB    | ~5.6MB   | OpenGL/Metal support |

## Troubleshooting

### Build Issues

**"GUI build disabled"**:
- Ensure CMake 3.16+ is installed
- Check internet connection (downloads dependencies)
- Try fallback: `-DGUI_USE_SDL2=OFF` to use GLFW

**Compilation errors**:
- Verify C++17 compiler support
- Update CMake to latest version
- Check available system memory (static builds are memory-intensive)

### Runtime Issues

**GUI doesn't start**:
- Verify OpenGL drivers are installed
- Check graphics hardware compatibility
- Try running from command line to see error messages

**COM port access denied**:
- **Linux**: Add user to `dialout` group: `sudo usermod -a -G dialout $USER`
- **Windows**: Run as Administrator or check port permissions

**Connection timeouts**:
- Verify target device is running and accessible
- Check firewall settings for GDB connections
- Increase timeout values for slow connections

## Compared to CLI Version

| Feature | CLI | GUI |
|---------|-----|-----|
| Size | 59KB | 5.4MB |
| Dependencies | None | OpenGL only |
| Real-time progress | No | Yes |
| Log visualization | Console only | Rich formatting |
| Settings persistence | Command line only | Automatic |
| Multi-platform | Yes | Yes |
| Ease of use | Technical users | All users |

## Contributing

The GUI is built using:
- **Dear ImGui**: https://github.com/ocornut/imgui
- **SDL2**: https://github.com/libsdl-org/SDL
- **GLFW**: https://github.com/glfw/glfw (alternative backend)

To contribute:
1. Fork the repository
2. Create feature branch: `git checkout -b feature/gui-enhancement`
3. Make changes and test on multiple platforms
4. Submit pull request

## License

Same as RTEgetData main project - MIT License.