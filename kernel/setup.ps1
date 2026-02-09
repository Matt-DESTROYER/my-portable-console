Write-Host "--- INSTALLING PICO DEVELOPMENT TOOLS ---" -ForegroundColor Cyan

Write-Host "Installing Git..." -ForegroundColor Cyan
winget install --id Git.Git -e --source winget

Write-Host "Installing Python..." -ForegroundColor Cyan
winget install --moniker python3 -e --source winget

Write-Host "Installing Visual Studio 2022 Build tools" -ForegroundColor Cyan
winget install --id Microsoft.VisualStudio.2022.BuildTools --override "--passive --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"

Write-Host "Installing Arm GNU Toolchain..." -ForegroundColor Cyan
winget install --id Arm.GnuArmEmbeddedToolchain -e --source winget

Write-Host "Installing Ninja..." -ForegroundColor Cyan
winget install --id Ninja-build.Ninja -e --source winget

Write-Host "Installing CMake..." -ForegroundColor Cyan
winget install --id Kitware.CMake -e --source winget

Write-Host "Installation complete!" -ForegroundColor Green
Write-Host "WARNING: You MUST close VS Code and this Terminal" -ForegroundColor Yellow
Write-Host "and re-open them for the new tools to be found." -ForegroundColor Yellow
