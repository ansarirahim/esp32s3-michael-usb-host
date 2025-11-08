@echo off
REM ESP32-S3 USB Host Automator - Build Script with IDF Environment
REM This script automatically initializes ESP-IDF environment and runs build commands

setlocal enabledelayedexpansion

REM ESP-IDF Installation Path
set "IDF_PATH=C:\Users\Abdul\esp-idf-v5.5.1"
set "IDF_TOOLS=%IDF_PATH%\export.bat"

REM Check if ESP-IDF exists
if not exist "%IDF_TOOLS%" (
    echo ERROR: ESP-IDF not found at %IDF_PATH%
    echo Please update the IDF_PATH variable in this script
    exit /b 1
)

if "%1"=="" (
    echo.
    echo ========================================
    echo ESP32-S3 USB Host Automator - Auto Build
    echo ========================================
    echo.
    echo Usage: build-idf.bat [command]
    echo.
    echo Commands:
    echo   clean           - Clean build (fullclean)
    echo   build           - Build firmware
    echo   flash-com14     - Flash to COM14 (GPIO38 - Espressif)
    echo   flash-com11     - Flash to COM11 (GPIO48 - EWeAct)
    echo   flash-both      - Flash to both COM14 and COM11
    echo   monitor-com14   - Monitor COM14 serial output
    echo   monitor-com11   - Monitor COM11 serial output
    echo   full            - Clean + Build + Flash COM14 + Monitor
    echo   full-both       - Clean + Build + Flash BOTH + Monitor COM14
    echo.
    echo Examples:
    echo   build-idf.bat build
    echo   build-idf.bat flash-both
    echo   build-idf.bat full-both
    echo.
    goto :eof
)

REM Initialize ESP-IDF environment
echo Initializing ESP-IDF environment...
call "%IDF_TOOLS%"
if errorlevel 1 (
    echo ERROR: Failed to initialize ESP-IDF
    exit /b 1
)
echo.

REM Execute commands
if "%1"=="clean" (
    echo Cleaning build...
    idf.py fullclean
    goto :eof
)

if "%1"=="build" (
    echo Building firmware...
    idf.py build
    goto :eof
)

if "%1"=="flash-com14" (
    echo Flashing to COM14 (GPIO38 - Espressif)...
    idf.py -p COM14 flash
    goto :eof
)

if "%1"=="flash-com11" (
    echo Flashing to COM11 (GPIO48 - EWeAct)...
    idf.py -p COM11 flash
    goto :eof
)

if "%1"=="flash-both" (
    echo Flashing to COM14 (GPIO38 - Espressif)...
    idf.py -p COM14 flash
    if errorlevel 1 (
        echo Failed to flash COM14
        goto :eof
    )
    echo.
    echo Flashing to COM11 (GPIO48 - EWeAct)...
    idf.py -p COM11 flash
    goto :eof
)

if "%1"=="monitor-com14" (
    echo Monitoring COM14 (GPIO38 - Espressif)...
    echo Press Ctrl+] to exit monitor
    idf.py -p COM14 monitor
    goto :eof
)

if "%1"=="monitor-com11" (
    echo Monitoring COM11 (GPIO48 - EWeAct)...
    echo Press Ctrl+] to exit monitor
    idf.py -p COM11 monitor
    goto :eof
)

if "%1"=="full" (
    echo.
    echo ========================================
    echo Full Build: Clean + Build + Flash COM14 + Monitor
    echo ========================================
    echo.
    idf.py fullclean
    if errorlevel 1 goto :eof
    idf.py build
    if errorlevel 1 goto :eof
    idf.py -p COM14 flash
    if errorlevel 1 goto :eof
    echo Press Ctrl+] to exit monitor
    idf.py -p COM14 monitor
    goto :eof
)

if "%1"=="full-both" (
    echo.
    echo ========================================
    echo Full Build: Clean + Build + Flash BOTH + Monitor COM14
    echo ========================================
    echo.
    idf.py fullclean
    if errorlevel 1 goto :eof
    idf.py build
    if errorlevel 1 goto :eof
    idf.py -p COM14 flash
    if errorlevel 1 goto :eof
    idf.py -p COM11 flash
    if errorlevel 1 goto :eof
    echo Press Ctrl+] to exit monitor
    idf.py -p COM14 monitor
    goto :eof
)

echo Unknown command: %1
echo Run "build-idf.bat" without arguments for help

