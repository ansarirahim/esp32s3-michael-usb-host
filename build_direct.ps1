# Phase 1 Build Script - PowerShell
Write-Host ""
Write-Host "========================================"
Write-Host "Phase 1: Building with PowerShell"
Write-Host "========================================"
Write-Host ""

# Set IDF_PATH
$env:IDF_PATH = "C:\Users\Abdul\esp-idf-v5.5.1"

# Change to project directory
Set-Location $PSScriptRoot

Write-Host "[1/4] Initializing ESP-IDF environment..."
& "$env:IDF_PATH\export.ps1"

Write-Host ""
Write-Host "[2/4] Cleaning build directory..."
if (Test-Path "build") {
    Remove-Item -Recurse -Force "build"
}

Write-Host ""
Write-Host "[3/4] Building firmware..."
& "$env:IDF_PATH\tools\idf.py" build

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "========================================"
    Write-Host "BUILD FAILED!"
    Write-Host "========================================"
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host ""
Write-Host "[4/4] Flashing to COM14..."
& "$env:IDF_PATH\tools\idf.py" -p COM14 flash

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "========================================"
    Write-Host "FLASH FAILED!"
    Write-Host "========================================"
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host ""
Write-Host "========================================"
Write-Host "SUCCESS! Check COM14 for new firmware"
Write-Host "========================================"
Write-Host ""
Read-Host "Press Enter to exit"

