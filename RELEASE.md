# RTEgetData Release Guide

## Quick Start for Users

### Download Pre-Built Binaries

1. Go to the [Releases page](https://github.com/YOUR_USERNAME/RTEgetData/releases)
2. Download the appropriate file for your operating system:
   - **Windows**: `RTEgetData-Windows-x64-MSVC.zip`
   - **Linux**: `RTEgetData-Linux-Ubuntu-Latest-x64.tar.gz`
   - **macOS**: `RTEgetData-macOS-x64.tar.gz`

### What's Included

Each download contains:
- **RTEgetData-CLI**: Command-line version (~60KB)
- **RTEgetData-GUI**: Graphical version (~5.3MB)
- **Documentation**: README, build instructions, changelog
- **Launcher scripts**: Easy startup files

### Running the Software

#### Windows
1. Extract the ZIP file
2. Double-click `RTEgetData-GUI.exe` for the graphical interface
3. Or run `RTEgetData-CLI.exe` from command prompt for CLI usage

#### Linux/macOS
1. Extract the tar.gz file: `tar -xzf RTEgetData-*.tar.gz`
2. Make executable: `chmod +x RTEgetData-*`
3. Run GUI: `./RTEgetData-GUI` or `./run-gui.sh`
4. Run CLI: `./RTEgetData-CLI --help` or `./run-cli.sh --help`

#### Linux COM Port Access
Add your user to the dialout group:
```bash
sudo usermod -a -G dialout $USER
# Log out and log back in
```

## For Maintainers

### Automated Release Process

#### 1. Tag-Based Releases (Recommended)
```bash
# Update CHANGELOG.md first
git add CHANGELOG.md
git commit -m "Update changelog for v2.0.0"

# Create and push tag
git tag v2.0.0
git push origin v2.0.0
```

This automatically:
- Builds all platform variants
- Creates GitHub release
- Uploads all artifacts
- Generates checksums

#### 2. Manual Release via GitHub Actions
1. Go to **Actions** → **Manual Release**
2. Click **Run workflow**
3. Enter version (e.g., `v2.0.0`)
4. Choose options (draft/prerelease)
5. Click **Run workflow**

### Build Matrix

The automated system builds:

| Platform | Compiler | Artifact Name |
|----------|----------|---------------|
| Windows x64 | MSVC | `RTEgetData-Windows-x64-MSVC.zip` |
| Windows x64 | MinGW | `RTEgetData-Windows-x64-MinGW.zip` |
| Ubuntu 20.04 | GCC | `RTEgetData-Linux-Ubuntu-20.04-x64.tar.gz` |
| Ubuntu 22.04 | GCC | `RTEgetData-Linux-Ubuntu-22.04-x64.tar.gz` |
| Ubuntu Latest | GCC | `RTEgetData-Linux-Ubuntu-Latest-x64.tar.gz` |
| macOS Latest | Clang | `RTEgetData-macOS-x64.tar.gz` |

### Release Checklist

Before creating a release:

- [ ] Update version information in `CHANGELOG.md`
- [ ] Test build scripts locally (`./build.sh`, `build.bat`)
- [ ] Verify GUI and CLI functionality
- [ ] Check cross-platform compatibility
- [ ] Update documentation if needed
- [ ] Create git tag with version number

### Version Numbers

- **Major**: `v2.0.0` - Breaking changes, new architecture
- **Minor**: `v2.1.0` - New features, backwards compatible
- **Patch**: `v2.0.1` - Bug fixes, small improvements
- **Pre-release**: `v2.0.0-rc1`, `v2.0.0-beta2`

### File Sizes (Approximate)

| Platform | CLI Size | GUI Size | Archive Size |
|----------|----------|----------|--------------|
| Windows | ~60KB | ~5.5MB | ~2.8MB |
| Linux | ~59KB | ~5.3MB | ~2.7MB |
| macOS | ~65KB | ~5.8MB | ~2.9MB |

All builds use static linking for zero dependencies.

### Troubleshooting Releases

#### Build Failures
1. Check **Actions** tab for error logs
2. Common issues:
   - Missing dependencies in build environment
   - CMake configuration problems
   - Platform-specific compilation errors

#### Testing Releases
1. Download artifacts from successful builds
2. Test on clean systems without dev tools
3. Verify both GUI and CLI work correctly
4. Test connection functionality (if possible)

#### Manual Release Recovery
If automated release fails:

1. Download all artifacts from the workflow run
2. Create archives manually:
   ```bash
   # Windows
   zip -r RTEgetData-Windows-x64-MSVC.zip RTEgetData-Windows-x64-MSVC/
   
   # Linux/macOS
   tar -czf RTEgetData-Linux-Ubuntu-Latest-x64.tar.gz RTEgetData-Linux-Ubuntu-Latest-x64/
   ```
3. Generate checksums: `sha256sum *.zip *.tar.gz > checksums.sha256`
4. Create GitHub release manually and upload files

### GitHub Repository Settings

Ensure these settings for proper releases:

1. **Settings** → **Actions** → **General**:
   - Allow GitHub Actions
   - Allow read and write permissions

2. **Settings** → **Pages** (optional):
   - Enable for documentation hosting

3. **Settings** → **Releases**:
   - Enable automatic release notes

### Development Workflow

1. **Feature Development**: Work on feature branches
2. **Testing**: Push triggers test builds on all platforms
3. **Integration**: Merge to main/master after testing
4. **Release**: Create tag to trigger full release build

### Support and Distribution

Released binaries are:
- **Self-contained**: No external dependencies
- **Portable**: Can be copied anywhere
- **Verified**: Include SHA256 checksums
- **Documented**: Include complete usage instructions

Users can simply download, extract, and run without installation.