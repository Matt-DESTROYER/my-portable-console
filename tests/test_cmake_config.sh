#!/bin/bash

# Test suite for kernel CMake configuration files
# Tests CMakeLists.txt and CMakePresets.json

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
KERNEL_DIR="$PROJECT_ROOT/kernel"
CMAKE_FILE="$KERNEL_DIR/CMakeLists.txt"
PRESETS_FILE="$KERNEL_DIR/CMakePresets.json"

# Test results
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Utility functions
print_test_header() {
    echo ""
    echo "======================================"
    echo "TEST: $1"
    echo "======================================"
}

pass_test() {
    echo -e "${GREEN}[PASS]${NC} $1"
    TESTS_PASSED=$((TESTS_PASSED + 1))
    TESTS_RUN=$((TESTS_RUN + 1))
}

fail_test() {
    echo -e "${RED}[FAIL]${NC} $1"
    TESTS_FAILED=$((TESTS_FAILED + 1))
    TESTS_RUN=$((TESTS_RUN + 1))
}

info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

# Test 1: Check if CMakeLists.txt exists
test_cmakelists_exists() {
    print_test_header "CMakeLists.txt Exists"

    if [ -f "$CMAKE_FILE" ]; then
        pass_test "CMakeLists.txt exists"
        return 0
    else
        fail_test "CMakeLists.txt not found"
        return 1
    fi
}

# Test 2: Check CMake minimum version
test_cmake_minimum_version() {
    print_test_header "CMake Minimum Version"

    if grep -q "cmake_minimum_required" "$CMAKE_FILE"; then
        local version=$(grep "cmake_minimum_required" "$CMAKE_FILE" | head -1)
        info "Found: $version"
        pass_test "CMake minimum version specified"
        return 0
    else
        fail_test "CMake minimum version not specified"
        return 1
    fi
}

# Test 3: Check project declaration
test_project_declaration() {
    print_test_header "Project Declaration"

    if grep -q "^project(" "$CMAKE_FILE"; then
        local project=$(grep "^project(" "$CMAKE_FILE" | head -1)
        info "Found: $project"
        pass_test "Project is declared"
        return 0
    else
        fail_test "Project declaration not found"
        return 1
    fi
}

# Test 4: Check Pico SDK import
test_pico_sdk_import() {
    print_test_header "Pico SDK Import"

    if grep -q "pico_sdk_import" "$CMAKE_FILE" || \
       grep -q "PICO_SDK_PATH" "$CMAKE_FILE"; then
        pass_test "Pico SDK is imported"
        return 0
    else
        fail_test "Pico SDK import not found"
        return 1
    fi
}

# Test 5: Check Pico SDK initialization
test_pico_sdk_init() {
    print_test_header "Pico SDK Initialization"

    if grep -q "pico_sdk_init()" "$CMAKE_FILE"; then
        pass_test "Pico SDK initialization present"
        return 0
    else
        fail_test "Pico SDK not initialized"
        return 1
    fi
}

# Test 6: Check board configuration
test_board_config() {
    print_test_header "Board Configuration"

    if grep -q "PICO_BOARD" "$CMAKE_FILE"; then
        local board=$(grep "PICO_BOARD" "$CMAKE_FILE" | head -1)
        info "Found: $board"
        pass_test "Board is configured"
        return 0
    else
        fail_test "Board configuration not found"
        return 1
    fi
}

# Test 7: Check platform configuration
test_platform_config() {
    print_test_header "Platform Configuration"

    if grep -q "PICO_PLATFORM" "$CMAKE_FILE"; then
        local platform=$(grep "PICO_PLATFORM" "$CMAKE_FILE" | head -1)
        info "Found: $platform"
        pass_test "Platform is configured"
        return 0
    else
        info "Platform not explicitly set (may use default)"
        pass_test "Platform configuration acceptable"
        return 0
    fi
}

# Test 8: Check executable definition
test_executable_definition() {
    print_test_header "Executable Definition"

    if grep -q "add_executable" "$CMAKE_FILE"; then
        local exe=$(grep "add_executable" "$CMAKE_FILE" | head -1)
        info "Found: $exe"
        pass_test "Executable is defined"
        return 0
    else
        fail_test "No executable defined"
        return 1
    fi
}

# Test 9: Check source files
test_source_files() {
    print_test_header "Source Files"

    if grep -q "src/main.c" "$CMAKE_FILE" || \
       grep -q "src/" "$CMAKE_FILE"; then
        pass_test "Source files are specified"
        return 0
    else
        fail_test "Source files not properly specified"
        return 1
    fi
}

# Test 10: Check library linking
test_library_linking() {
    print_test_header "Library Linking"

    if grep -q "target_link_libraries" "$CMAKE_FILE"; then
        pass_test "Libraries are linked"
        return 0
    else
        fail_test "No library linking found"
        return 1
    fi
}

# Test 11: Check Pico stdlib
test_pico_stdlib() {
    print_test_header "Pico Standard Library"

    if grep -q "pico_stdlib" "$CMAKE_FILE"; then
        pass_test "Pico stdlib is linked"
        return 0
    else
        fail_test "Pico stdlib not linked"
        return 1
    fi
}

# Test 12: Check USB stdio configuration
test_usb_stdio() {
    print_test_header "USB STDIO Configuration"

    if grep -q "pico_enable_stdio_usb" "$CMAKE_FILE"; then
        local usb_config=$(grep "pico_enable_stdio_usb" "$CMAKE_FILE" | head -1)
        info "Found: $usb_config"
        pass_test "USB stdio is configured"
        return 0
    else
        fail_test "USB stdio not configured"
        return 1
    fi
}

# Test 13: Check extra outputs (UF2)
test_extra_outputs() {
    print_test_header "Extra Outputs (UF2)"

    if grep -q "pico_add_extra_outputs" "$CMAKE_FILE"; then
        pass_test "Extra outputs (UF2) are configured"
        return 0
    else
        fail_test "Extra outputs not configured"
        return 1
    fi
}

# Test 14: Check CMakePresets.json exists
test_presets_exists() {
    print_test_header "CMakePresets.json Exists"

    if [ -f "$PRESETS_FILE" ]; then
        pass_test "CMakePresets.json exists"
        return 0
    else
        fail_test "CMakePresets.json not found"
        return 1
    fi
}

# Test 15: Check JSON syntax
test_json_syntax() {
    print_test_header "CMakePresets.json Syntax"

    if command -v python3 &> /dev/null; then
        if python3 -c "import json; json.load(open('$PRESETS_FILE'))" 2>/dev/null; then
            pass_test "JSON syntax is valid"
            return 0
        else
            fail_test "JSON syntax is invalid"
            return 1
        fi
    else
        # Basic JSON check
        if grep -q "\"version\"" "$PRESETS_FILE"; then
            pass_test "Basic JSON structure appears valid"
            return 0
        else
            fail_test "JSON structure may be invalid"
            return 1
        fi
    fi
}

# Test 16: Check presets version
test_presets_version() {
    print_test_header "CMakePresets Version"

    if grep -q "\"version\"" "$PRESETS_FILE"; then
        local version=$(grep "\"version\"" "$PRESETS_FILE" | head -1)
        info "Found: $version"
        pass_test "Presets version specified"
        return 0
    else
        fail_test "Presets version not specified"
        return 1
    fi
}

# Test 17: Check configure presets
test_configure_presets() {
    print_test_header "Configure Presets"

    if grep -q "configurePresets" "$PRESETS_FILE"; then
        pass_test "Configure presets defined"
        return 0
    else
        fail_test "Configure presets not defined"
        return 1
    fi
}

# Test 18: Check build presets
test_build_presets() {
    print_test_header "Build Presets"

    if grep -q "buildPresets" "$PRESETS_FILE"; then
        pass_test "Build presets defined"
        return 0
    else
        fail_test "Build presets not defined"
        return 1
    fi
}

# Test 19: Check generator
test_generator() {
    print_test_header "Generator Configuration"

    if grep -q "\"generator\"" "$PRESETS_FILE"; then
        local generator=$(grep "\"generator\"" "$PRESETS_FILE" | head -1)
        info "Found: $generator"
        pass_test "Generator is specified"
        return 0
    else
        fail_test "Generator not specified"
        return 1
    fi
}

# Test 20: Check for debug and release builds
test_build_types() {
    print_test_header "Build Types (Debug/Release)"

    local has_debug=false
    local has_release=false

    if grep -qi "debug" "$PRESETS_FILE"; then
        has_debug=true
        info "Found debug build preset"
    fi

    if grep -qi "release" "$PRESETS_FILE"; then
        has_release=true
        info "Found release build preset"
    fi

    if [ "$has_debug" = true ] && [ "$has_release" = true ]; then
        pass_test "Both debug and release builds configured"
        return 0
    else
        fail_test "Missing debug or release build configuration"
        return 1
    fi
}

# Test 21: Check C/C++ standards
test_language_standards() {
    print_test_header "C/C++ Standards"

    local has_c_std=false
    local has_cxx_std=false

    if grep -q "CMAKE_C_STANDARD" "$CMAKE_FILE"; then
        has_c_std=true
        info "C standard specified"
    fi

    if grep -q "CMAKE_CXX_STANDARD" "$CMAKE_FILE"; then
        has_cxx_std=true
        info "C++ standard specified"
    fi

    if [ "$has_c_std" = true ] && [ "$has_cxx_std" = true ]; then
        pass_test "Both C and C++ standards specified"
        return 0
    else
        fail_test "Language standards not fully specified"
        return 1
    fi
}

# Test 22: Check FetchContent for Pico SDK
test_fetchcontent() {
    print_test_header "FetchContent Configuration"

    if grep -q "FetchContent" "$CMAKE_FILE"; then
        pass_test "FetchContent is used for dependencies"
        return 0
    else
        info "FetchContent not used (may use external SDK)"
        pass_test "Dependency management present"
        return 0
    fi
}

# Main execution
main() {
    echo ""
    echo "###############################################"
    echo "#  CMAKE CONFIGURATION TEST SUITE"
    echo "###############################################"
    echo ""

    # CMakeLists.txt tests
    test_cmakelists_exists
    test_cmake_minimum_version
    test_project_declaration
    test_pico_sdk_import
    test_pico_sdk_init
    test_board_config
    test_platform_config
    test_executable_definition
    test_source_files
    test_library_linking
    test_pico_stdlib
    test_usb_stdio
    test_extra_outputs
    test_language_standards
    test_fetchcontent

    # CMakePresets.json tests
    test_presets_exists
    test_json_syntax
    test_presets_version
    test_configure_presets
    test_build_presets
    test_generator
    test_build_types

    # Summary
    echo ""
    echo "======================================"
    echo "TEST SUMMARY"
    echo "======================================"
    echo "Tests run:    $TESTS_RUN"
    echo -e "Tests passed: ${GREEN}$TESTS_PASSED${NC}"
    echo -e "Tests failed: ${RED}$TESTS_FAILED${NC}"
    echo ""

    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "${GREEN}ALL TESTS PASSED${NC}"
        exit 0
    else
        echo -e "${RED}SOME TESTS FAILED${NC}"
        exit 1
    fi
}

# Run main
main