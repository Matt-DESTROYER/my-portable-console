# Test Suite

This directory contains comprehensive tests for the build configuration, scripts, and workflows in this project.

## Test Files

### 1. `test_build.sh`
Tests the kernel build script (`kernel/build.sh`)

**Coverage:**
- Script existence and syntax validation
- Clean command functionality
- Error handling and exit codes
- CMake preset usage
- Build directory management
- Script structure and completeness

**Run:** `bash tests/test_build.sh`

**Tests:** 12 tests

### 2. `test_setup.ps1`
Tests the Windows setup script (`kernel/setup.ps1`)

**Coverage:**
- Script existence and syntax validation
- Required tools installation (Git, Python, ARM toolchain, Ninja, CMake)
- Winget package manager usage
- Command availability checks
- User feedback and colored output
- Skip logic for already-installed tools
- Restart/reopen warnings

**Run:** `powershell tests/test_setup.ps1` (Windows only)

**Tests:** 15 tests

### 3. `test_workflow.sh`
Tests the GitHub Actions release workflow (`.github/workflows/release.yml`)

**Coverage:**
- YAML syntax validation
- Workflow trigger configuration (v* tags)
- Permissions (contents, id-token, attestations)
- Job steps (checkout, dependencies, build, attest, upload)
- Runner configuration (ubuntu-latest)
- Build attestation
- Release notes generation
- Security checks (no hardcoded secrets)
- Conditional execution

**Run:** `bash tests/test_workflow.sh`

**Tests:** 20 tests

### 4. `test_cmake_config.sh`
Tests CMake configuration files (`kernel/CMakeLists.txt` and `kernel/CMakePresets.json`)

**Coverage:**
- CMake syntax and structure
- Project declaration and versioning
- Pico SDK integration and initialization
- Board and platform configuration (Pico 2, RP2350)
- Source files and executable definition
- Library linking (pico_stdlib, hardware_spi)
- USB stdio configuration
- UF2 output generation
- CMake presets (default, debug, release)
- Generator configuration (Ninja)
- Language standards (C11, C++17)

**Run:** `bash tests/test_cmake_config.sh`

**Tests:** 22 tests

## Test Summary

| Test Suite | Tests | Platform |
|------------|-------|----------|
| test_build.sh | 12 | Linux/macOS |
| test_setup.ps1 | 15 | Windows |
| test_workflow.sh | 20 | Linux/macOS |
| test_cmake_config.sh | 22 | Linux/macOS |
| **Total** | **69** | - |

## Running All Tests

### Linux/macOS
```bash
# Run all bash-based tests
bash tests/test_build.sh
bash tests/test_workflow.sh
bash tests/test_cmake_config.sh
```

### Windows
```powershell
# Run PowerShell tests
powershell tests/test_setup.ps1

# Run bash tests (if WSL or Git Bash available)
bash tests/test_build.sh
bash tests/test_workflow.sh
bash tests/test_cmake_config.sh
```

## Test Results

All tests include:
- ✅ Pass/fail indicators with colored output
- 📊 Detailed test summaries
- 📝 Informational messages for context
- 🚨 Clear failure messages with context

## SD Card Test

The `sd-card-test/` directory contains a hardware integration test for SD card functionality:

**File:** `sd-card-test/sd-card-test.c`

**Coverage:**
- SPI initialization and communication
- SD card detection
- Command protocol (CMD0, CMD8, ACMD41, CMD58)
- Voltage range validation
- Card capacity detection (SDHC/SDXC/SDSC)
- OCR register reading
- Test result tracking and reporting

**Build:**
```bash
cd sd-card-test
cmake -B build
cmake --build build
```

**Run:** Flash the generated `sd-card-test.uf2` file to a Raspberry Pi Pico 2 board and monitor via USB serial.

## Test Design Principles

1. **Comprehensive Coverage**: Tests cover main functionality and edge cases
2. **Clear Output**: Color-coded results with detailed information
3. **Non-Destructive**: Tests validate without modifying production code
4. **Self-Contained**: Each test suite can run independently
5. **Informative Failures**: Failed tests provide context for debugging
6. **Platform Appropriate**: Tests use platform-native tools

## Continuous Integration

The GitHub Actions workflow (`.github/workflows/release.yml`) automatically:
- Runs on tag pushes (v*)
- Executes setup and build scripts
- Generates build attestations
- Creates GitHub releases with artifacts

The workflow itself is validated by `test_workflow.sh`.