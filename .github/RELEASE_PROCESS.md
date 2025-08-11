# Release Process for RTEgetData

This document outlines the automated release process for RTEgetData using GitHub Actions.

## Automated Build System

The project uses GitHub Actions to automatically build cross-platform releases for:

- **Windows** (MSVC and MinGW compilers)
- **Linux** (Ubuntu 20.04, 22.04, and latest)
- **macOS** (latest)

Both CLI and GUI versions are built for all platforms with static linking for zero-dependency distribution.

## Release Workflow

### 1. Development Builds

Every push to feature branches triggers test builds to ensure cross-platform compatibility:

```bash
git push origin feat/my-feature
```

This runs quick compilation tests on all platforms without creating artifacts.

### 2. Creating a Release

To create a new release with downloadable binaries:

1. **Update Version Information**:
   - Update `CHANGELOG.md` with new version and changes
   - Ensure version information is accurate

2. **Create and Push a Tag**:
   ```bash
   git tag v2.0.0
   git push origin v2.0.0
   ```

3. **Automatic Release Process**:
   - GitHub Actions automatically detects the tag
   - Builds all platform variants
   - Creates release archives
   - Publishes GitHub Release with artifacts

### 3. Release Artifacts

The automated process creates the following downloadable files:

#### Windows
- `RTEgetData-Windows-x64-MSVC.zip`
  - `RTEgetData-CLI.exe` (~60KB)
  - `RTEgetData-GUI.exe` (~5.5MB)
  - Documentation files

- `RTEgetData-Windows-x64-MinGW.zip`
  - Alternative MinGW build for compatibility

#### Linux
- `RTEgetData-Linux-Ubuntu-20.04-x64.tar.gz`
- `RTEgetData-Linux-Ubuntu-22.04-x64.tar.gz`  
- `RTEgetData-Linux-Ubuntu-Latest-x64.tar.gz`

Each contains:
- `RTEgetData-CLI` (~59KB)
- `RTEgetData-GUI` (~5.3MB)
- `run-cli.sh` and `run-gui.sh` launcher scripts
- Documentation files

#### macOS
- `RTEgetData-macOS-x64.tar.gz`
  - Similar structure to Linux builds
  - Universal compatibility for Intel Macs

### 4. Release Notes

Release notes are automatically generated from `CHANGELOG.md`. The system:

1. Extracts the section for the current version
2. Includes it in the GitHub Release description
3. Falls back to generic text if no specific section is found

### 5. Verification

Each release includes:
- **Checksums**: `checksums.sha256` file for integrity verification
- **Build summary**: Detailed status of each platform build
- **Automatic testing**: CLI help functionality verified on all platforms

## Manual Release Steps (if needed)

If you need to create a release manually:

1. **Download Artifacts**: From the GitHub Actions workflow run
2. **Create Archives**: Zip/tar.gz the artifacts appropriately
3. **Generate Checksums**: 
   ```bash
   sha256sum *.zip *.tar.gz > checksums.sha256
   ```
4. **Create GitHub Release**: Upload files and checksums

## Version Naming Convention

- **Stable releases**: `v2.0.0`, `v2.1.0`
- **Pre-releases**: `v2.0.0-rc1`, `v2.0.0-beta2`
- **Development**: Automatic builds on feature branches

## Build Configuration

All builds use:
- **Static linking**: Zero external dependencies
- **Release optimization**: Maximum performance
- **Both CLI and GUI**: Complete functionality
- **Cross-platform**: Same features on all platforms

## Troubleshooting Builds

### Build Failures
- Check the GitHub Actions logs for specific errors
- Common issues: missing dependencies, CMake configuration problems
- The workflow includes fallback options for different scenarios

### Platform-Specific Issues
- **Windows**: MSVC vs MinGW compatibility differences
- **Linux**: Different Ubuntu versions may have varying library versions
- **macOS**: Xcode Command Line Tools required

### Testing Releases
- Download artifacts from successful workflow runs
- Test on clean systems without development tools
- Verify both CLI and GUI functionality

## Contributing to Build System

The build configuration files are:
- `.github/workflows/build-release.yml`: Main release workflow
- `.github/workflows/test-build.yml`: Development testing
- `CMakeLists.txt`: Build system configuration
- `build.sh`, `build.bat`: Local build scripts

Changes to the build system should be tested on feature branches before merging to main.

## Release Schedule

- **Major releases**: New features, breaking changes
- **Minor releases**: Feature additions, improvements  
- **Patch releases**: Bug fixes, small improvements

All releases are automatically built and published through the GitHub Actions workflow.