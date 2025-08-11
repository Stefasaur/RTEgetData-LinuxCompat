# RTEgetData CI/CD Documentation

## Overview

RTEgetData uses GitHub Actions for comprehensive continuous integration and deployment, automatically building cross-platform binaries for Windows, Linux, and macOS.

## Workflow Files

### 1. `build-release.yml` - Main Release Pipeline
**Triggers:**
- Tags matching `v*` (e.g., `v2.0.0`)
- Pushes to main/master branches
- Pull requests to main/master
- Manual dispatch

**What it does:**
- Builds for Windows (MSVC + MinGW), Linux (multiple distros), macOS
- Creates downloadable artifacts
- Publishes GitHub releases for tags
- Generates checksums and build summaries

### 2. `test-build.yml` - Development Testing
**Triggers:**
- Pushes to feature branches
- Pull requests

**What it does:**
- Quick compilation tests on all platforms
- Validates build system works
- No artifact creation (faster feedback)

### 3. `manual-release.yml` - On-Demand Releases
**Triggers:**
- Manual workflow dispatch with parameters

**What it does:**
- Create releases with custom version numbers
- Support for draft/pre-release options
- Full artifact creation and upload

### 4. `validate-workflows.yml` - Workflow Validation
**Triggers:**
- Changes to workflow files

**What it does:**
- YAML syntax validation
- File permission checks

## Build Matrix

| Platform | OS Version | Compiler | Output |
|----------|------------|----------|---------|
| Windows | latest | MSVC 2022 | `RTEgetData-Windows-x64-MSVC.zip` |
| Windows | latest | MinGW-w64 | `RTEgetData-Windows-x64-MinGW.zip` |
| Linux | Ubuntu 20.04 | GCC | `RTEgetData-Linux-Ubuntu-20.04-x64.tar.gz` |
| Linux | Ubuntu 22.04 | GCC | `RTEgetData-Linux-Ubuntu-22.04-x64.tar.gz` |
| Linux | Ubuntu Latest | GCC | `RTEgetData-Linux-Ubuntu-Latest-x64.tar.gz` |
| macOS | latest | Clang | `RTEgetData-macOS-x64.tar.gz` |

## Artifact Contents

Each platform package includes:
- **Executables**: `RTEgetData-CLI` and `RTEgetData-GUI`
- **Documentation**: `Readme.md`, `LICENSE.md`, `CHANGELOG.md`, `BUILD.md`
- **Launcher scripts**: Platform-specific startup helpers
- **Static linking**: Zero external dependencies

## Release Process

### Automatic (Recommended)
1. Update `CHANGELOG.md` with version changes
2. Create and push a git tag:
   ```bash
   git tag v2.0.0
   git push origin v2.0.0
   ```
3. GitHub Actions automatically builds and publishes

### Manual via GitHub UI
1. Go to **Actions** → **Manual Release**
2. Click **Run workflow**
3. Enter version number and options
4. Click **Run workflow**

## Build Configuration

### CMake Settings
All builds use:
```cmake
-DCMAKE_BUILD_TYPE=Release
-DBUILD_GUI=ON
-DGUI_USE_SDL2=ON  
-DGUI_STATIC_LINK=ON
```

### Static Linking
- **Windows**: Uses `/MT` flag and static runtime
- **Linux/macOS**: Static linking of GUI dependencies
- **Result**: Self-contained executables

### Parallel Building
- **Windows**: Uses all available cores
- **Linux**: `$(nproc)` cores
- **macOS**: `$(sysctl -n hw.ncpu)` cores

## Dependencies

### Build-Time Dependencies
Automatically installed by workflows:
- **CMake 3.16+**
- **Compilers**: MSVC/MinGW (Windows), GCC (Linux), Clang (macOS)
- **System libraries**: OpenGL, X11 (Linux)

### Downloaded Dependencies
Fetched automatically by CMake:
- **Dear ImGui 1.90.9**
- **SDL2 2.28.5** (or GLFW 3.3.8 as fallback)

## Workflow Secrets and Permissions

### Required Permissions
- **Contents**: Read (for code checkout)
- **Actions**: Write (for workflow runs)
- **Releases**: Write (for publishing releases)

### No Secrets Needed
The workflows use GitHub's automatic `GITHUB_TOKEN` - no manual secret configuration required.

## Platform-Specific Notes

### Windows
- **MSVC Build**: Uses Visual Studio 2022 (latest)
- **MinGW Build**: Uses MSYS2 with MinGW-w64 toolchain
- **Dependencies**: Automatically links `ws2_32.dll` for networking

### Linux
- **Multi-distro**: Tests across Ubuntu versions for compatibility
- **Dependencies**: Installs `build-essential`, `cmake`, `libgl1-mesa-dev`
- **Container**: Uses Docker containers for older Ubuntu versions

### macOS
- **Architecture**: Intel x64 (Apple Silicon support can be added)
- **Dependencies**: Uses Homebrew for CMake and pkg-config
- **Compatibility**: macOS 10.12+ supported

## Troubleshooting

### Build Failures

**Common Issues:**
1. **CMake configuration fails**: Check CMake version and dependencies
2. **Compilation errors**: Platform-specific code issues
3. **Linking failures**: Missing libraries or incorrect flags

**Debugging Steps:**
1. Check the **Actions** tab for detailed logs
2. Look for specific error messages in build output
3. Test locally with the same build scripts (`build.sh`, `build.bat`)

### Release Issues

**Missing Artifacts:**
- Verify all build jobs completed successfully
- Check if upload-artifact steps executed properly

**Release Creation Fails:**
- Ensure tag follows `v*` pattern
- Check repository permissions for releases
- Verify changelog format for automatic extraction

### Performance Optimization

**Build Speed:**
- Parallel compilation enabled on all platforms
- Artifact uploads are concurrent
- Dependencies are cached where possible

**Storage:**
- Artifacts retained for 90 days (releases) or 30 days (manual)
- Checksums provided for integrity verification

## Extending the Build System

### Adding New Platforms
1. Add new matrix entry to `build-release.yml`
2. Install platform-specific dependencies
3. Configure CMake for the new platform
4. Update packaging steps

### Adding New Build Variants
1. Extend strategy matrix with new configuration
2. Add conditional steps for variant-specific setup
3. Update artifact naming scheme
4. Test across all platforms

### Modifying Build Configuration
1. Update CMake options in workflow files
2. Test changes on feature branches first
3. Update documentation to match changes

## Monitoring and Maintenance

### Build Health
- **Success Rate**: Monitor workflow success/failure rates
- **Build Times**: Track performance across platforms
- **Artifact Sizes**: Monitor for unexpected size increases

### Security
- **Dependency Updates**: Monitor for security updates in fetched dependencies
- **Workflow Updates**: Keep GitHub Actions up to date
- **Permission Reviews**: Regularly review workflow permissions

### Documentation
- Keep this file updated when workflows change
- Update user-facing documentation for new releases
- Maintain changelog with CI/CD improvements

## Local Testing

Before pushing changes, test locally:
```bash
# Test build scripts
./build.sh        # Linux/macOS
./build.bat       # Windows

# Verify executables
./build/RTEgetData-CLI --help
./build/RTEgetData-GUI
```

This ensures workflow builds will succeed.