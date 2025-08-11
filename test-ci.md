# CI/CD Test Instructions

## Testing the Fixed Workflow

The GitHub Actions build failed on Windows MinGW. Here are the fixes applied:

### Changes Made:

1. **Enhanced MinGW Setup**:
   - Added `update: true` to ensure latest packages
   - Added `mingw-w64-x86_64-pkg-config` and `mingw-w64-x86_64-opengl`
   - Set explicit PKG_CONFIG_PATH

2. **Improved CMake Configuration**:
   - Added fallback from SDL2 to GLFW if SDL2 fails
   - Set explicit compilers (`-DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++`)
   - Added verbose output for debugging

3. **Better Build Verification**:
   - Added post-build checks to verify executables exist
   - Enhanced error reporting

4. **CMakeLists.txt Improvements**:
   - Added MinGW-specific linker flags and libraries
   - Set proper Windows subsystem for GUI application
   - Added MinGW-specific compiler definitions

### Testing Steps:

1. **Push the changes**:
   ```bash
   git add .
   git commit -m "Fix Windows MinGW build issues"
   git push origin main
   ```

2. **Create a debug branch** (optional):
   ```bash
   git checkout -b debug-build
   git push origin debug-build
   ```
   This will trigger the `debug-build.yml` workflow for detailed diagnostics.

3. **Manual testing** (if you have Windows):
   - Test `build.bat` (MSVC)
   - Test `build-mingw.bat` (MinGW)

### Expected Fixes:

- **MinGW Library Linking**: Added system libraries needed for Windows GUI
- **Compiler Detection**: Explicit compiler setting should resolve toolchain issues  
- **Fallback Backend**: If SDL2 fails, GLFW will be tried automatically
- **Build Verification**: Clear error messages if executables aren't created

### Common Issues Addressed:

1. **Missing OpenGL libraries**: Added mingw-w64-x86_64-opengl
2. **pkg-config not found**: Added explicit package and PATH
3. **Linker errors**: Added Windows-specific system libraries
4. **CMake generator issues**: Using Ninja consistently for MinGW
5. **Static linking problems**: Proper subsystem and library specifications

### Debugging Info:

The `debug-build.yml` workflow provides:
- Environment checks (compiler versions, paths)
- Step-by-step build process
- Verbose build output
- Artifact upload for failed builds

If builds still fail, check the Actions logs for:
- CMake configuration errors
- Compilation errors  
- Linker errors
- Missing dependencies

### Next Steps:

1. Monitor the GitHub Actions run
2. If still failing, check the detailed logs
3. Use the debug workflow for more information
4. May need platform-specific adjustments

The fixes target the most common Windows MinGW + static linking + GUI framework issues.