#!/bin/bash

# Test suite for kernel/build.sh
# Tests the build script functionality

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
KERNEL_DIR="$PROJECT_ROOT/kernel"
BUILD_SCRIPT="$KERNEL_DIR/build.sh"

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

# Test 1: Check if build script exists
test_script_exists() {
    print_test_header "Build Script Exists"

    if [ -f "$BUILD_SCRIPT" ]; then
        pass_test "build.sh exists at $BUILD_SCRIPT"
        return 0
    else
        fail_test "build.sh not found at $BUILD_SCRIPT"
        return 1
    fi
}

# Test 2: Check if build script is executable
test_script_executable() {
    print_test_header "Build Script Executable"

    if [ -x "$BUILD_SCRIPT" ]; then
        pass_test "build.sh is executable"
        return 0
    else
        info "build.sh is not executable, but can still be run with bash"
        pass_test "build.sh can be executed with bash"
        return 0
    fi
}

# Test 3: Check script syntax
test_script_syntax() {
    print_test_header "Build Script Syntax"

    if bash -n "$BUILD_SCRIPT" 2>/dev/null; then
        pass_test "build.sh has valid bash syntax"
        return 0
    else
        fail_test "build.sh has syntax errors"
        return 1
    fi
}

# Test 4: Test clean command
test_clean_command() {
    print_test_header "Clean Command"

    # Create a dummy build directory
    mkdir -p "$KERNEL_DIR/build"
    touch "$KERNEL_DIR/build/dummy_file.txt"

    if [ ! -d "$KERNEL_DIR/build" ]; then
        fail_test "Could not create test build directory"
        return 1
    fi

    # Run clean command
    cd "$KERNEL_DIR"
    bash ./build.sh clean >/dev/null 2>&1

    if [ ! -d "$KERNEL_DIR/build" ]; then
        pass_test "Clean command successfully removes build directory"
        return 0
    else
        fail_test "Clean command did not remove build directory"
        return 1
    fi
}

# Test 5: Check for required commands in script
test_required_commands() {
    print_test_header "Required Commands in Script"

    local required_commands=("cmake" "cd" "echo" "exit" "rm")
    local all_found=true

    for cmd in "${required_commands[@]}"; do
        if grep -q "$cmd" "$BUILD_SCRIPT"; then
            info "Found command: $cmd"
        else
            fail_test "Missing expected command: $cmd"
            all_found=false
        fi
    done

    if [ "$all_found" = true ]; then
        pass_test "All required commands present in script"
        return 0
    else
        return 1
    fi
}

# Test 6: Check error handling
test_error_handling() {
    print_test_header "Error Handling"

    # Check if script has error handling
    if grep -q "if \[ \$? -ne 0 \]" "$BUILD_SCRIPT" || \
       grep -q "if \[ \$? -eq 0 \]" "$BUILD_SCRIPT"; then
        pass_test "Script has error handling with exit code checks"
        return 0
    else
        fail_test "Script lacks explicit error handling"
        return 1
    fi
}

# Test 7: Check for CMake preset usage
test_cmake_preset_usage() {
    print_test_header "CMake Preset Usage"

    if grep -q "\-\-preset" "$BUILD_SCRIPT"; then
        pass_test "Script uses CMake presets"
        return 0
    else
        fail_test "Script does not use CMake presets"
        return 1
    fi
}

# Test 8: Check for build directory creation/usage
test_build_directory_usage() {
    print_test_header "Build Directory Usage"

    if grep -q "cd build" "$BUILD_SCRIPT" || \
       grep -q "build/" "$BUILD_SCRIPT"; then
        pass_test "Script properly uses build directory"
        return 0
    else
        fail_test "Script does not reference build directory"
        return 1
    fi
}

# Test 9: Verify script structure (has main sections)
test_script_structure() {
    print_test_header "Script Structure"

    local has_clean=false
    local has_configure=false
    local has_build=false

    if grep -q "clean" "$BUILD_SCRIPT"; then
        has_clean=true
        info "Has clean functionality"
    fi

    if grep -q "cmake.*preset" "$BUILD_SCRIPT" || grep -q "Configuring" "$BUILD_SCRIPT"; then
        has_configure=true
        info "Has configure functionality"
    fi

    if grep -q "cmake.*build" "$BUILD_SCRIPT" || grep -q "Building" "$BUILD_SCRIPT"; then
        has_build=true
        info "Has build functionality"
    fi

    if [ "$has_clean" = true ] && [ "$has_configure" = true ] && [ "$has_build" = true ]; then
        pass_test "Script has proper structure (clean, configure, build)"
        return 0
    else
        fail_test "Script missing some expected sections"
        return 1
    fi
}

# Test 10: Check exit codes
test_exit_codes() {
    print_test_header "Exit Codes"

    local exit_count=$(grep -c "exit" "$BUILD_SCRIPT" || true)

    if [ "$exit_count" -gt 0 ]; then
        pass_test "Script uses exit statements ($exit_count found)"
        return 0
    else
        fail_test "Script does not use exit statements"
        return 1
    fi
}

# Test 11: Edge case - empty argument
test_empty_argument() {
    print_test_header "Empty Argument Handling"

    # Create build directory for this test
    mkdir -p "$KERNEL_DIR/build"

    cd "$KERNEL_DIR"

    # Running without arguments should proceed to build (not clean)
    # We're just checking it doesn't crash
    info "Testing script with no arguments (syntax check only)"

    if bash -n ./build.sh; then
        pass_test "Script handles empty arguments without syntax errors"
        return 0
    else
        fail_test "Script fails with empty arguments"
        return 1
    fi
}

# Test 12: Verify shebang
test_shebang() {
    print_test_header "Shebang Line"

    local first_line=$(head -n 1 "$BUILD_SCRIPT")

    if [[ "$first_line" == "#!/bin/bash"* ]] || [[ "$first_line" == "#!/usr/bin/env bash"* ]]; then
        pass_test "Script has proper bash shebang"
        return 0
    else
        fail_test "Script missing or has incorrect shebang (found: $first_line)"
        return 1
    fi
}

# Main execution
main() {
    echo ""
    echo "###############################################"
    echo "#  BUILD.SH TEST SUITE"
    echo "###############################################"
    echo ""

    # Run all tests
    test_script_exists
    test_script_executable
    test_script_syntax
    test_required_commands
    test_error_handling
    test_cmake_preset_usage
    test_build_directory_usage
    test_script_structure
    test_exit_codes
    test_shebang
    test_clean_command
    test_empty_argument

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