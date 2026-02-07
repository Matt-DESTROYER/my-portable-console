param (
	[string]$Command
)

# --- CLEAN COMMAND ---
if ($command -eq "clean") {
	Write-Host "Cleaning build directory..."
	if (Test-Path "build") {
		Remove-Item -Recurse -Force "build"
	}
	Write-Host "Done."
	exit 0
}

# --- COMPILER AUTO-DISCOVERY
try {
	$null = Get-Command arm-none-eabi-gcc -ErrorAction Stop
} catch {
	Write-Host "Compiler not found in PATH. Searching in standard locations..." -ForegroundColor Yellow

	$SearchPaths = @(
		"C:\Program Files\Arm GNU Toolchain arm-none-eabi",
		"C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi",
		"C:\Program Files\GNU Arm Embedded Toolchain",
		"C:\Program Files (x86)\GNU Arm Embedded Toolchain"
	)

	$FoundPath = $null

	foreach ($Path in $SearchPaths) {
		if (Test-Path $Path) {
			$Version = Get-ChildItem -Path $Path -Directory | Select-Object -First 1
			if ($Version) {
				$BinPath = Join-Path $Version.FullName "bin"
				if (Test-Path "$BinPath\arm-none-eabi-gcc.exe") {
					$FoundPath = $BinPath
					break
				}
			}
		}
	}

	if ($FoundPath) {
		Write-Host "Found compiler at: $FoundPath" -ForegroundColor Green
		$env:PATH = "$FoundPath;$env:PATH"
	} else {
		Write-Host "Error: Could not find 'arm-none-eabi-gcc'..." -ForegroundColor Red
		Write-Host "Please restart your terminal if you just ran the setup script." -ForegroundColor Yellow
		exit 1
	}
}

# --- BUILD ---
# configure
Write-Host "Configuring project..."
cmake --preset default

if ($LASTEXITCODE -ne 0) {
	Write-Host "CMake configuration failed..." -ForegroundColor Red
	exit 1
}

# build
Write-Host "Building project..."
cmake --build --preset release

if ($LASTEXITCODE -eq 0) {
	Write-Host "Build success!" -ForegroundColor Green
} else {
	Write-Host "Build failed..." -ForegroundColor Red
	exit 1
}
