# ESP32-S3 USB Host Automator - Easy Build Script (PowerShell)
# Usage: build-easy [command]
# Or just: build-easy (for help)

param(
    [string]$Command = ""
)

# Get script directory
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

function Show-Help {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "ESP32-S3 USB Host Automator - Build Tool" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Usage: build-easy [command]" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Commands:" -ForegroundColor Green
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
    Write-Host "Examples:" -ForegroundColor Green
    Write-Host "  build-easy clean" -ForegroundColor Gray
    Write-Host "  build-easy build" -ForegroundColor Gray
    Write-Host "  build-easy flash-both" -ForegroundColor Gray
    Write-Host "  build-easy full-both" -ForegroundColor Gray
    Write-Host ""
}

function Invoke-Command {
    param([string]$Cmd)
    Write-Host "Executing: $Cmd" -ForegroundColor Yellow
    Invoke-Expression $Cmd
    return $LASTEXITCODE
}

if ([string]::IsNullOrEmpty($Command)) {
    Show-Help
    exit 0
}

switch ($Command.ToLower()) {
    "clean" {
        Write-Host "Cleaning build..." -ForegroundColor Cyan
        Invoke-Command "idf.py fullclean"
    }
    
    "build" {
        Write-Host "Building firmware..." -ForegroundColor Cyan
        Invoke-Command "idf.py build"
    }
    
    "flash-com14" {
        Write-Host "Flashing to COM14 (GPIO38 - Espressif)..." -ForegroundColor Cyan
        Invoke-Command "idf.py -p COM14 flash"
    }
    
    "flash-com11" {
        Write-Host "Flashing to COM11 (GPIO48 - EWeAct)..." -ForegroundColor Cyan
        Invoke-Command "idf.py -p COM11 flash"
    }
    
    "flash-both" {
        Write-Host "Flashing to COM14 (GPIO38 - Espressif)..." -ForegroundColor Cyan
        $result = Invoke-Command "idf.py -p COM14 flash"
        if ($result -ne 0) {
            Write-Host "Failed to flash COM14" -ForegroundColor Red
            exit 1
        }
        Write-Host ""
        Write-Host "Flashing to COM11 (GPIO48 - EWeAct)..." -ForegroundColor Cyan
        Invoke-Command "idf.py -p COM11 flash"
    }
    
    "monitor-com14" {
        Write-Host "Monitoring COM14 (GPIO38 - Espressif)..." -ForegroundColor Cyan
        Invoke-Command "idf.py -p COM14 monitor"
    }
    
    "monitor-com11" {
        Write-Host "Monitoring COM11 (GPIO48 - EWeAct)..." -ForegroundColor Cyan
        Invoke-Command "idf.py -p COM11 monitor"
    }
    
    "full" {
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host "Full Build: Clean + Build + Flash COM14 + Monitor" -ForegroundColor Cyan
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host ""
        
        $result = Invoke-Command "idf.py fullclean"
        if ($result -ne 0) { exit 1 }
        
        $result = Invoke-Command "idf.py build"
        if ($result -ne 0) { exit 1 }
        
        $result = Invoke-Command "idf.py -p COM14 flash"
        if ($result -ne 0) { exit 1 }
        
        Invoke-Command "idf.py -p COM14 monitor"
    }
    
    "full-both" {
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host "Full Build: Clean + Build + Flash BOTH + Monitor COM14" -ForegroundColor Cyan
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host ""
        
        $result = Invoke-Command "idf.py fullclean"
        if ($result -ne 0) { exit 1 }
        
        $result = Invoke-Command "idf.py build"
        if ($result -ne 0) { exit 1 }
        
        $result = Invoke-Command "idf.py -p COM14 flash"
        if ($result -ne 0) { exit 1 }
        
        $result = Invoke-Command "idf.py -p COM11 flash"
        if ($result -ne 0) { exit 1 }
        
        Invoke-Command "idf.py -p COM14 monitor"
    }
    
    default {
        Write-Host "Unknown command: $Command" -ForegroundColor Red
        Write-Host "Run 'build-easy' without arguments for help" -ForegroundColor Yellow
        exit 1
    }
}

