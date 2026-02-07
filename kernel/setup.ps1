Write-Host "--- INSTALLING PICO DEVELOPMENT TOOLS ---" -ForegroundColor Cyan

if (Get-Command git -ErrorAction SilentlyContinue) {
	Write-Host "Git installed, skipping..."
} else {
	Write-Host "Installing Git..."
	winget install --id Git.Git -e --source winget
}

if (Get-Command python -ErrorAction SilentlyContinue) {
	Write-Host "Python installed, skipping..."
} else {
	Write-Host "Installing Python..."
	winget install --moniker python3 -e --source winget
}

Write-Host "Installing Arm GNU Toolchain..."
winget install --id Arm.GnuArmEmbeddedToolchain -e --source winget

Write-Host "Installing Ninja..."
winget install --id Ninja-build.Ninja -e --source winget

Write-Host "Installing CMake..."
winget install --id Kitware.CMake -e --source winget

Write-Host "Installation complete!" -ForegroundColor Green
Write-Host "WARNING: You MUST close VS Code and this Terminal" -ForegroundColor Yellow
Write-Host "and re-open them for the new tools to be found." -ForegroundColor Yellow
