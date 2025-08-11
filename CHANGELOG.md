# Changelog

All notable changes to the RTEgetData project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2025-08-11 (Unreleased)

### Added
- **Complete Linux Support**: Full cross-platform compatibility for Linux systems
- **Modern GUI Interface**: Dear ImGui-based graphical user interface with real-time progress tracking
- **Dual Backend Support**: Both SDL2 and GLFW backends for maximum compatibility
- **Static Linking**: Zero-dependency executables for easy distribution
- **Automatic COM Port Detection**: Dropdown selection with device descriptions on both Windows and Linux
- **Real-Time Logging**: Live message display with timestamps and color-coded severity levels
- **Background Operations**: Non-blocking data transfers with progress visualization
- **Connection Management**: Retry capability and proper state management
- **Comprehensive Tooltips**: Beginner-friendly help system aligned with official documentation
- **Cross-Platform Build System**: CMake-based build system supporting Windows, Linux, and macOS
- **Automated Build Scripts**: One-click build scripts for all platforms (`build.sh`, `build.bat`, `build-mingw.bat`)
- **Settings Persistence**: Automatic save/restore of connection settings (TODO)
- **Recent Connections**: Quick access to previously used connections (TODO)

### Changed
- **Architecture**: Refactored from Windows-only Visual Studio project to cross-platform CMake
- **COM Port Implementation**: Full Linux serial port support using POSIX termios API (replaces stubs)
- **Error Handling**: Enhanced cross-platform error reporting and socket management
- **Build System**: Modern CMake 3.16+ with FetchContent for dependency management
- **Dependencies**: Automatic downloading and static linking of GUI frameworks
- **File Structure**: Organized code with proper platform abstraction layer

### Fixed
- **Linux Compatibility**: Resolved all Windows-specific includes and functions
- **Socket Errors**: Proper error code mapping between Windows (WSA) and Linux (errno)
- **Memory Management**: Thread-safe operations and proper resource cleanup
- **Connection Stability**: Fixed single-connection limitation, now supports reconnection
- **GUI Layout**: Improved from nested windows to integrated single-window design

### Technical Details

#### New Files
- `platform_compat.h/cpp`: Cross-platform compatibility layer
- `RTEgetData_GUI.h/cpp`: Complete GUI implementation
- `RTEgetData_GUI_main.cpp`: GUI application entry point
- `CMakeLists.txt`: Modern build system configuration
- `build.sh`: Linux/macOS automated build script
- `build.bat`: Windows MSVC automated build script
- `build-mingw.bat`: Windows MinGW automated build script
- `BUILD.md`: Comprehensive build documentation
- `GUI_README.md`: GUI-specific usage documentation
- `CHANGELOG.md`: This changelog file

#### Enhanced Modules
- **com_lib.cpp**: Full Linux serial communication with extensive baud rate support
- **gdb_lib.cpp**: Cross-platform TCP socket implementation
- **logger.cpp**: Platform-aware timing and file operations
- **bridge.cpp**: Cross-platform process management and error handling
- **cmd_line.cpp**: Enhanced parameter parsing for both Windows and Linux paths

#### Build Targets
- **RTEgetData**: CLI version (~59-60KB)
- **RTEgetData-GUI**: GUI version (~5.3-5.5MB, fully self-contained)

#### Dependencies (Automatically Downloaded)
- Dear ImGui 1.90.9: Immediate mode GUI framework
- SDL2 2.28.5: Cross-platform window/input management (default backend)
- GLFW 3.3.8: Alternative window management backend
- OpenGL: Hardware-accelerated rendering (system-provided)

#### Supported Platforms
- **Linux**: Ubuntu, Debian, CentOS, RHEL, Arch Linux, and most distributions
- **Windows**: Windows 7+ with Visual Studio 2019+ or MinGW-w64
- **macOS**: macOS 10.12+ with Xcode Command Line Tools

#### COM Port Support
- **Windows**: Native Windows API with SetupAPI enumeration (`COM1`, `COM2`, etc.)
- **Linux**: POSIX termios API with device scanning (`/dev/ttyUSB0`, `/dev/ttyACM0`, etc.)

#### GDB Server Support
- **J-Link**: Default port 2331, parallel operation with IDE debugger
- **ST-LINK**: Default port 61234, full protocol compatibility
- **OpenOCD**: Default port 3333, configurable probe selection

#### Performance Improvements
- **Parallel Building**: Multi-core compilation support
- **Static Linking**: Single-executable deployment
- **Memory Efficiency**: Optimized buffer management
- **Connection Speed**: Improved timeout handling and retry logic

### Breaking Changes
- **Build System**: Visual Studio `.sln/.vcxproj` files deprecated in favor of CMake
- **Dependencies**: Manual dependency management replaced with automatic FetchContent
- **Platform**: Windows-only limitation removed, now requires cross-platform code practices

### Migration Guide
- **From Visual Studio**: Use `build.bat` or CMake directly instead of opening `.sln` files
- **Command Line**: CLI interface unchanged, full backward compatibility maintained
- **Binaries**: New executables have same functionality with cross-platform support

## [1.0.0] - 2024 (Original Release)

### Added
- Initial Windows-only implementation
- Visual Studio project support
- GDB server protocol support (J-Link, ST-LINK, OpenOCD)
- Windows COM port communication
- Command-line interface
- Binary data transfer capabilities
- RTEdbg logging toolkit integration
- Post-mortem and single-shot data logging modes
- Automatic logging restart functionality
- Filter management and buffer clearing
- Batch file execution support

### Features
- **GDB Server Support**: TCP/IP communication with debug probes
- **COM Port Support**: Direct serial communication (Windows only)
- **Data Transfer**: Memory reading with configurable address and size
- **Logging Integration**: Full RTEdbg toolkit compatibility
- **Error Handling**: Comprehensive error reporting and recovery
- **Performance**: Priority-based process management for reliable transfers

### Supported Debug Probes
- Segger J-Link (GDB server port 2331)
- STMicroelectronics ST-LINK (GDB server port 61234)  
- OpenOCD compatible probes (GDB server port 3333)
- PE Micro Multilink
- ESP32 JTAG debugging

### Technical Specifications
- **Platform**: Windows only (Visual Studio 2019+)
- **Architecture**: x86/x64 Windows executables
- **Dependencies**: Windows Sockets (Winsock2), Windows API
- **Size**: Approximately 60KB executable
- **Requirements**: Windows 7+, debug probe with GDB server

---

## Version History Summary

- **v2.0.0**: Complete rewrite with cross-platform support, modern GUI, and enhanced functionality
- **v1.0.0**: Original Windows-only command-line implementation

## Contributors

- **B. Premzel**: Original author and maintainer

## License

MIT License - See [LICENSE.md](LICENSE.md) for details.

## Related Projects

- [RTEdbg Logging Toolkit](https://github.com/RTEdbg/RTEdbg): Complete embedded logging solution
- [RTEdbg Releases](https://github.com/RTEdbg/RTEdbg/releases): Official toolkit distributions

---

*For detailed build instructions, see [BUILD.md](BUILD.md)*  
*For GUI usage guide, see [GUI_README.md](GUI_README.md)*  
*For general usage, see [Readme.md](Readme.md)*