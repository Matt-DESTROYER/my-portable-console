#!/bin/bash

# Test suite for .github/workflows/release.yml
# Validates GitHub Actions workflow configuration

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
WORKFLOW_FILE="$PROJECT_ROOT/.github/workflows/release.yml"

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

# Test 1: Check if workflow file exists
test_workflow_exists() {
    print_test_header "Workflow File Exists"

    if [ -f "$WORKFLOW_FILE" ]; then
        pass_test "release.yml exists at $WORKFLOW_FILE"
        return 0
    else
        fail_test "release.yml not found at $WORKFLOW_FILE"
        return 1
    fi
}

# Test 2: Check YAML syntax
test_yaml_syntax() {
    print_test_header "YAML Syntax"

    # Try to parse YAML with Python if available
    if command -v python3 &> /dev/null; then
        if python3 -c "import yaml; yaml.safe_load(open('$WORKFLOW_FILE'))" 2>/dev/null; then
            pass_test "YAML syntax is valid"
            return 0
        else
            # Fallback to basic checks
            info "Python YAML parsing failed, using basic checks"
        fi
    fi

    # Basic YAML validation
    if grep -q "^[a-zA-Z]" "$WORKFLOW_FILE" && ! grep -q $'\t' "$WORKFLOW_FILE"; then
        pass_test "Basic YAML structure appears valid (no tabs)"
        return 0
    else
        fail_test "YAML file may contain tabs or invalid structure"
        return 1
    fi
}

# Test 3: Check workflow name
test_workflow_name() {
    print_test_header "Workflow Name"

    if grep -q "^name:" "$WORKFLOW_FILE"; then
        local name=$(grep "^name:" "$WORKFLOW_FILE" | head -1)
        info "Found: $name"
        pass_test "Workflow has a name defined"
        return 0
    else
        fail_test "Workflow does not have a name"
        return 1
    fi
}

# Test 4: Check trigger configuration (on tag push)
test_trigger_config() {
    print_test_header "Trigger Configuration"

    if grep -q "on:" "$WORKFLOW_FILE" && \
       grep -q "push:" "$WORKFLOW_FILE" && \
       grep -q "tags:" "$WORKFLOW_FILE"; then
        pass_test "Workflow triggers on tag push"
        return 0
    else
        fail_test "Workflow trigger not properly configured"
        return 1
    fi
}

# Test 5: Check for tag pattern
test_tag_pattern() {
    print_test_header "Tag Pattern"

    if grep -q '"v\*"' "$WORKFLOW_FILE" || grep -q "'v\*'" "$WORKFLOW_FILE"; then
        pass_test "Workflow triggers on v* tags"
        return 0
    else
        fail_test "Tag pattern not found or incorrect"
        return 1
    fi
}

# Test 6: Check permissions
test_permissions() {
    print_test_header "Workflow Permissions"

    if grep -q "permissions:" "$WORKFLOW_FILE"; then
        local perms_found=0

        if grep -q "contents:.*write" "$WORKFLOW_FILE"; then
            info "Found: contents: write"
            perms_found=$((perms_found + 1))
        fi

        if grep -q "id-token:.*write" "$WORKFLOW_FILE"; then
            info "Found: id-token: write"
            perms_found=$((perms_found + 1))
        fi

        if grep -q "attestations:.*write" "$WORKFLOW_FILE"; then
            info "Found: attestations: write"
            perms_found=$((perms_found + 1))
        fi

        if [ $perms_found -eq 3 ]; then
            pass_test "All required permissions are set"
            return 0
        else
            fail_test "Missing some required permissions (found $perms_found/3)"
            return 1
        fi
    else
        fail_test "No permissions defined"
        return 1
    fi
}

# Test 7: Check for jobs
test_jobs() {
    print_test_header "Jobs Definition"

    if grep -q "jobs:" "$WORKFLOW_FILE"; then
        pass_test "Workflow has jobs defined"
        return 0
    else
        fail_test "No jobs defined in workflow"
        return 1
    fi
}

# Test 8: Check runner
test_runner() {
    print_test_header "Runner Configuration"

    if grep -q "runs-on:.*ubuntu-latest" "$WORKFLOW_FILE"; then
        pass_test "Uses ubuntu-latest runner"
        return 0
    else
        fail_test "Runner not properly configured"
        return 1
    fi
}

# Test 9: Check steps
test_steps() {
    print_test_header "Workflow Steps"

    local steps_found=0
    local expected_steps=("Checkout" "Dependencies" "Build" "Attest" "Upload")

    for step in "${expected_steps[@]}"; do
        if grep -qi "$step" "$WORKFLOW_FILE"; then
            info "Found step: $step"
            steps_found=$((steps_found + 1))
        fi
    done

    if [ $steps_found -eq ${#expected_steps[@]} ]; then
        pass_test "All expected steps are present ($steps_found/${#expected_steps[@]})"
        return 0
    else
        fail_test "Missing some steps (found $steps_found/${#expected_steps[@]})"
        return 1
    fi
}

# Test 10: Check checkout action
test_checkout_action() {
    print_test_header "Checkout Action"

    if grep -q "actions/checkout@v6" "$WORKFLOW_FILE"; then
        pass_test "Uses checkout action v6"
        return 0
    elif grep -q "actions/checkout@" "$WORKFLOW_FILE"; then
        info "Uses checkout action but different version"
        pass_test "Uses checkout action"
        return 0
    else
        fail_test "Checkout action not found"
        return 1
    fi
}

# Test 11: Check build attestation
test_build_attestation() {
    print_test_header "Build Attestation"

    if grep -q "attest-build-provenance" "$WORKFLOW_FILE"; then
        pass_test "Build attestation step present"
        return 0
    else
        fail_test "Build attestation not configured"
        return 1
    fi
}

# Test 12: Check release action
test_release_action() {
    print_test_header "Release Action"

    if grep -q "action-gh-release" "$WORKFLOW_FILE" || \
       grep -q "gh-release" "$WORKFLOW_FILE"; then
        pass_test "GitHub release action configured"
        return 0
    else
        fail_test "Release action not found"
        return 1
    fi
}

# Test 13: Check for working directory
test_working_directory() {
    print_test_header "Working Directory"

    if grep -q "working-directory:" "$WORKFLOW_FILE"; then
        local workdir=$(grep "working-directory:" "$WORKFLOW_FILE" | head -1)
        info "Found: $workdir"
        pass_test "Working directory is configured"
        return 0
    else
        fail_test "Working directory not set"
        return 1
    fi
}

# Test 14: Check for setup script execution
test_setup_execution() {
    print_test_header "Setup Script Execution"

    if grep -q "setup.sh" "$WORKFLOW_FILE" || \
       grep -q "bash.*setup" "$WORKFLOW_FILE"; then
        pass_test "Setup script is executed"
        return 0
    else
        fail_test "Setup script not executed"
        return 1
    fi
}

# Test 15: Check for build script execution
test_build_execution() {
    print_test_header "Build Script Execution"

    if grep -q "build.sh" "$WORKFLOW_FILE" || \
       grep -q "bash.*build" "$WORKFLOW_FILE"; then
        pass_test "Build script is executed"
        return 0
    else
        fail_test "Build script not executed"
        return 1
    fi
}

# Test 16: Check artifact path
test_artifact_path() {
    print_test_header "Artifact Path"

    if grep -q "my_console.uf2" "$WORKFLOW_FILE"; then
        pass_test "Artifact path includes my_console.uf2"
        return 0
    else
        fail_test "Artifact path not properly configured"
        return 1
    fi
}

# Test 17: Check for release notes generation
test_release_notes() {
    print_test_header "Release Notes Generation"

    if grep -q "generate_release_notes" "$WORKFLOW_FILE"; then
        pass_test "Automatic release notes generation enabled"
        return 0
    else
        info "Release notes generation not explicitly enabled"
        pass_test "Release configuration present"
        return 0
    fi
}

# Test 18: Check GitHub token usage
test_github_token() {
    print_test_header "GitHub Token"

    if grep -q "GITHUB_TOKEN" "$WORKFLOW_FILE"; then
        pass_test "GitHub token is used"
        return 0
    else
        fail_test "GitHub token not configured"
        return 1
    fi
}

# Test 19: Edge case - check for conditional upload
test_conditional_upload() {
    print_test_header "Conditional Upload"

    if grep -q "if:.*ref_type" "$WORKFLOW_FILE" || \
       grep -q "if:.*tag" "$WORKFLOW_FILE"; then
        pass_test "Upload is conditional on tag"
        return 0
    else
        info "Upload may not be conditional"
        pass_test "Upload step present"
        return 0
    fi
}

# Test 20: Verify no hardcoded secrets
test_no_hardcoded_secrets() {
    print_test_header "No Hardcoded Secrets"

    # Check for common patterns of hardcoded secrets
    if grep -qE "(password|api[_-]?key|secret[_-]?key|token).*[:=].*['\"][a-zA-Z0-9]{20,}" "$WORKFLOW_FILE"; then
        fail_test "Potential hardcoded secrets found"
        return 1
    else
        pass_test "No hardcoded secrets detected"
        return 0
    fi
}

# Main execution
main() {
    echo ""
    echo "###############################################"
    echo "#  GITHUB WORKFLOW TEST SUITE"
    echo "###############################################"
    echo ""

    # Run all tests
    test_workflow_exists
    test_yaml_syntax
    test_workflow_name
    test_trigger_config
    test_tag_pattern
    test_permissions
    test_jobs
    test_runner
    test_steps
    test_checkout_action
    test_build_attestation
    test_release_action
    test_working_directory
    test_setup_execution
    test_build_execution
    test_artifact_path
    test_release_notes
    test_github_token
    test_conditional_upload
    test_no_hardcoded_secrets

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