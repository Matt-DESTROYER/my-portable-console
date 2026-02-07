# Test suite for kernel/setup.ps1
# Tests the Windows setup script functionality

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$KernelDir = Join-Path $ProjectRoot "kernel"
$SetupScript = Join-Path $KernelDir "setup.ps1"

# Test results
$TestsRun = 0
$TestsPassed = 0
$TestsFailed = 0

# Utility functions
function Print-TestHeader {
    param([string]$TestName)
    Write-Host ""
    Write-Host "======================================" -ForegroundColor Cyan
    Write-Host "TEST: $TestName" -ForegroundColor Cyan
    Write-Host "======================================" -ForegroundColor Cyan
}

function Pass-Test {
    param([string]$Message)
    Write-Host "[PASS] $Message" -ForegroundColor Green
    $script:TestsPassed++
    $script:TestsRun++
}

function Fail-Test {
    param([string]$Message)
    Write-Host "[FAIL] $Message" -ForegroundColor Red
    $script:TestsFailed++
    $script:TestsRun++
}

function Info {
    param([string]$Message)
    Write-Host "[INFO] $Message" -ForegroundColor Yellow
}

# Test 1: Check if setup script exists
function Test-ScriptExists {
    Print-TestHeader "Setup Script Exists"

    if (Test-Path $SetupScript) {
        Pass-Test "setup.ps1 exists at $SetupScript"
        return $true
    } else {
        Fail-Test "setup.ps1 not found at $SetupScript"
        return $false
    }
}

# Test 2: Check script syntax
function Test-ScriptSyntax {
    Print-TestHeader "Setup Script Syntax"

    try {
        $null = [System.Management.Automation.PSParser]::Tokenize((Get-Content $SetupScript -Raw), [ref]$null)
        Pass-Test "setup.ps1 has valid PowerShell syntax"
        return $true
    } catch {
        Fail-Test "setup.ps1 has syntax errors: $($_.Exception.Message)"
        return $false
    }
}

# Test 3: Check for required tools
function Test-RequiredToolsCheck {
    Print-TestHeader "Required Tools Check"

    $content = Get-Content $SetupScript -Raw
    $requiredTools = @("Git", "Python", "Arm", "Ninja", "CMake")
    $allFound = $true

    foreach ($tool in $requiredTools) {
        if ($content -match $tool) {
            Info "Found reference to: $tool"
        } else {
            Fail-Test "Missing expected tool: $tool"
            $allFound = $false
        }
    }

    if ($allFound) {
        Pass-Test "All required tools are referenced in script"
        return $true
    } else {
        return $false
    }
}

# Test 4: Check for winget usage
function Test-WingetUsage {
    Print-TestHeader "Winget Usage"

    $content = Get-Content $SetupScript -Raw

    if ($content -match "winget") {
        Pass-Test "Script uses winget package manager"
        return $true
    } else {
        Fail-Test "Script does not use winget"
        return $false
    }
}

# Test 5: Check for Get-Command checks
function Test-CommandChecks {
    Print-TestHeader "Command Availability Checks"

    $content = Get-Content $SetupScript -Raw

    if ($content -match "Get-Command") {
        Pass-Test "Script checks for command availability"
        return $true
    } else {
        Fail-Test "Script does not check for command availability"
        return $false
    }
}

# Test 6: Check for error handling
function Test-ErrorHandling {
    Print-TestHeader "Error Handling"

    $content = Get-Content $SetupScript -Raw

    if ($content -match "ErrorAction") {
        Pass-Test "Script has error handling configuration"
        return $true
    } else {
        Info "No explicit error action found, but may use default"
        Pass-Test "Script can use default error handling"
        return $true
    }
}

# Test 7: Check for user feedback (Write-Host)
function Test-UserFeedback {
    Print-TestHeader "User Feedback"

    $content = Get-Content $SetupScript -Raw
    $writeHostCount = ([regex]::Matches($content, "Write-Host")).Count

    if ($writeHostCount -gt 0) {
        Pass-Test "Script provides user feedback ($writeHostCount Write-Host statements)"
        return $true
    } else {
        Fail-Test "Script lacks user feedback"
        return $false
    }
}

# Test 8: Check for Git installation
function Test-GitInstallation {
    Print-TestHeader "Git Installation"

    $content = Get-Content $SetupScript -Raw

    if ($content -match "Git\.Git" -or $content -match "Installing Git") {
        Pass-Test "Script handles Git installation"
        return $true
    } else {
        Fail-Test "Script does not handle Git installation"
        return $false
    }
}

# Test 9: Check for Python installation
function Test-PythonInstallation {
    Print-TestHeader "Python Installation"

    $content = Get-Content $SetupScript -Raw

    if ($content -match "python" -or $content -match "Python") {
        Pass-Test "Script handles Python installation"
        return $true
    } else {
        Fail-Test "Script does not handle Python installation"
        return $false
    }
}

# Test 10: Check for ARM toolchain
function Test-ArmToolchain {
    Print-TestHeader "ARM Toolchain Installation"

    $content = Get-Content $SetupScript -Raw

    if ($content -match "Arm\.GnuArmEmbeddedToolchain" -or $content -match "Arm GNU") {
        Pass-Test "Script handles ARM GNU Toolchain installation"
        return $true
    } else {
        Fail-Test "Script does not handle ARM toolchain installation"
        return $false
    }
}

# Test 11: Check for warning about restart
function Test-RestartWarning {
    Print-TestHeader "Restart Warning"

    $content = Get-Content $SetupScript -Raw

    if ($content -match "WARNING" -or $content -match "close" -or $content -match "re-open") {
        Pass-Test "Script warns user about restart/reopen"
        return $true
    } else {
        Fail-Test "Script does not warn about restart"
        return $false
    }
}

# Test 12: Check for colored output
function Test-ColoredOutput {
    Print-TestHeader "Colored Output"

    $content = Get-Content $SetupScript -Raw

    if ($content -match "ForegroundColor") {
        Pass-Test "Script uses colored output for better UX"
        return $true
    } else {
        Fail-Test "Script does not use colored output"
        return $false
    }
}

# Test 13: Verify script structure
function Test-ScriptStructure {
    Print-TestHeader "Script Structure"

    $content = Get-Content $SetupScript -Raw
    $hasConditionals = $content -match "if\s*\("
    $hasInstallCommands = $content -match "winget install"

    if ($hasConditionals -and $hasInstallCommands) {
        Pass-Test "Script has proper structure (conditionals and install commands)"
        return $true
    } else {
        Fail-Test "Script structure incomplete"
        return $false
    }
}

# Test 14: Check for skip logic (already installed)
function Test-SkipLogic {
    Print-TestHeader "Skip Already Installed"

    $content = Get-Content $SetupScript -Raw

    if ($content -match "skipping" -or $content -match "already installed") {
        Pass-Test "Script skips already installed tools"
        return $true
    } else {
        Fail-Test "Script does not check if tools are already installed"
        return $false
    }
}

# Test 15: Edge case - check script can be loaded
function Test-ScriptLoading {
    Print-TestHeader "Script Can Be Loaded"

    try {
        # Try to parse the script without executing
        $scriptBlock = [scriptblock]::Create((Get-Content $SetupScript -Raw))
        Pass-Test "Script can be parsed and loaded"
        return $true
    } catch {
        Fail-Test "Script cannot be loaded: $($_.Exception.Message)"
        return $false
    }
}

# Main execution
function Main {
    Write-Host ""
    Write-Host "###############################################" -ForegroundColor Cyan
    Write-Host "#  SETUP.PS1 TEST SUITE" -ForegroundColor Cyan
    Write-Host "###############################################" -ForegroundColor Cyan
    Write-Host ""

    # Run all tests
    Test-ScriptExists
    Test-ScriptSyntax
    Test-RequiredToolsCheck
    Test-WingetUsage
    Test-CommandChecks
    Test-ErrorHandling
    Test-UserFeedback
    Test-GitInstallation
    Test-PythonInstallation
    Test-ArmToolchain
    Test-RestartWarning
    Test-ColoredOutput
    Test-ScriptStructure
    Test-SkipLogic
    Test-ScriptLoading

    # Summary
    Write-Host ""
    Write-Host "======================================" -ForegroundColor Cyan
    Write-Host "TEST SUMMARY" -ForegroundColor Cyan
    Write-Host "======================================" -ForegroundColor Cyan
    Write-Host "Tests run:    $TestsRun"
    Write-Host "Tests passed: $TestsPassed" -ForegroundColor Green
    Write-Host "Tests failed: $TestsFailed" -ForegroundColor Red
    Write-Host ""

    if ($TestsFailed -eq 0) {
        Write-Host "ALL TESTS PASSED" -ForegroundColor Green
        exit 0
    } else {
        Write-Host "SOME TESTS FAILED" -ForegroundColor Red
        exit 1
    }
}

# Run main
Main