# ESP32-S3 USB Host Automator - Auto Build Script with IDF Environment
# This script automatically initializes ESP-IDF environment and runs build commands

param(
    [Parameter(Position=0)]
    [string]$Command = ""
)

# ESP-IDF Installation Path
$IDF_PATH = "C:\Users\Abdul\esp-idf-v5.5.1"
$IDF_TOOLS_PATH = "$IDF_PATH\export.ps1"
$IDF_PY = "$IDF_PATH\tools\idf.py"

# Check if ESP-IDF exists
if (-not (Test-Path $IDF_TOOLS_PATH)) {
    Write-Host "ERROR: ESP-IDF not found at $IDF_PATH" -ForegroundColor Red
    Write-Host "Please update the IDF_PATH variable in this script" -ForegroundColor Yellow
    exit 1
}

function Show-Help {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "ESP32-S3 USB Host Automator - Auto Build" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Usage: .\build-auto.ps1 [command]" -ForegroundColor White
    Write-Host ""
    Write-Host "Commands:" -ForegroundColor Yellow
    Write-Host "  clean           - Clean build (fullclean)" -ForegroundColor White
    Write-Host "  build           - Build firmware" -ForegroundColor White
    Write-Host "  flash-com14     - Flash to COM14 (GPIO38 - Espressif)" -ForegroundColor White
    Write-Host "  flash-com11     - Flash to COM11 (GPIO48 - EWeAct)" -ForegroundColor White
    Write-Host "  flash-both      - Flash to both COM14 and COM11" -ForegroundColor White
    Write-Host "  monitor-com14   - Monitor COM14 serial output" -ForegroundColor White
    Write-Host "  monitor-com11   - Monitor COM11 serial output" -ForegroundColor White
    Write-Host "  full            - Clean + Build + Flash COM14 + Monitor" -ForegroundColor White
    Write-Host "  full-both       - Clean + Build + Flash BOTH + Monitor COM14" -ForegroundColor White
    Write-Host ""
    Write-Host "Examples:" -ForegroundColor Yellow
    Write-Host "  .\build-auto.ps1 build" -ForegroundColor Gray
    Write-Host "  .\build-auto.ps1 flash-both" -ForegroundColor Gray
    Write-Host "  .\build-auto.ps1 full-both" -ForegroundColor Gray
    Write-Host ""
}

function Run-IDFCommand {
    param([string]$IDFArgs)

    Write-Host "Running: idf.py $IDFArgs" -ForegroundColor Yellow

    # Run idf.py with the ESP-IDF environment in a single command
    $scriptBlock = @"
. '$IDF_TOOLS_PATH'
python '$IDF_PY' $IDFArgs
"@

    $result = powershell -NoProfile -ExecutionPolicy Bypass -Command $scriptBlock
    Write-Output $result

    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Command failed with exit code $LASTEXITCODE" -ForegroundColor Red
        return $false
    }
    return $true
}

# Show help if no command
if ($Command -eq "") {
    Show-Help
    exit 0
}

Write-Host "Initializing ESP-IDF environment..." -ForegroundColor Cyan
Write-Host ""

# Execute commands
switch ($Command) {
    "clean" {
        Write-Host "Cleaning build..." -ForegroundColor Cyan
        Run-IDFCommand "fullclean"
    }

    "build" {
        Write-Host "Building firmware..." -ForegroundColor Cyan
        Run-IDFCommand "build"
    }

    "flash-com14" {
        Write-Host "Flashing to COM14 (GPIO38 - Espressif)..." -ForegroundColor Cyan
        Run-IDFCommand "-p COM14 flash"
    }

    "flash-com11" {
        Write-Host "Flashing to COM11 (GPIO48 - EWeAct)..." -ForegroundColor Cyan
        Run-IDFCommand "-p COM11 flash"
    }

    "flash-both" {
        Write-Host "Flashing to COM14 (GPIO38 - Espressif)..." -ForegroundColor Cyan
        if (Run-IDFCommand "-p COM14 flash") {
            Write-Host ""
            Write-Host "Flashing to COM11 (GPIO48 - EWeAct)..." -ForegroundColor Cyan
            Run-IDFCommand "-p COM11 flash"
        }
    }

    "monitor-com14" {
        Write-Host "Monitoring COM14 (GPIO38 - Espressif)..." -ForegroundColor Cyan
        Write-Host "Press Ctrl+] to exit monitor" -ForegroundColor Yellow
        Run-IDFCommand "-p COM14 monitor"
    }

    "monitor-com11" {
        Write-Host "Monitoring COM11 (GPIO48 - EWeAct)..." -ForegroundColor Cyan
        Write-Host "Press Ctrl+] to exit monitor" -ForegroundColor Yellow
        Run-IDFCommand "-p COM11 monitor"
    }

    "full" {
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host "Full Build: Clean + Build + Flash COM14 + Monitor" -ForegroundColor Cyan
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host ""

        if (Run-IDFCommand "fullclean") {
            if (Run-IDFCommand "build") {
                if (Run-IDFCommand "-p COM14 flash") {
                    Write-Host "Press Ctrl+] to exit monitor" -ForegroundColor Yellow
                    Run-IDFCommand "-p COM14 monitor"
                }
            }
        }
    }

    "full-both" {
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host "Full Build: Clean + Build + Flash BOTH + Monitor COM14" -ForegroundColor Cyan
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host ""

        if (Run-IDFCommand "fullclean") {
            if (Run-IDFCommand "build") {
                if (Run-IDFCommand "-p COM14 flash") {
                    if (Run-IDFCommand "-p COM11 flash") {
                        Write-Host "Press Ctrl+] to exit monitor" -ForegroundColor Yellow
                        Run-IDFCommand "-p COM14 monitor"
                    }
                }
            }
        }
    }

    default {
        Write-Host "ERROR: Unknown command '$Command'" -ForegroundColor Red
        Write-Host ""
        Show-Help
        exit 1
    }
}

Write-Host ""
Write-Host "Done!" -ForegroundColor Green

