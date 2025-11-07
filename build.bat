@echo off
REM ESP32-S3 USB Host Automator - Build Script
REM Usage: build.bat [clean|build|flash-com14|flash-com11|flash-both|monitor-com14|monitor-com11]

setlocal enabledelayedexpansion

if "%1"=="" (
    echo.
    echo ========================================
    echo ESP32-S3 USB Host Automator - Build Tool
    echo ========================================
    echo.
    echo Usage: build.bat [command]
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
    echo   build.bat clean
    echo   build.bat build
    echo   build.bat flash-com14
    echo   build.bat flash-both
    echo   build.bat full
    echo.
    goto :eof
)

if "%1"=="clean" (
    echo Cleaning build...
    call idf.py fullclean
    goto :eof
)

if "%1"=="build" (
    echo Building firmware...
    call idf.py build
    goto :eof
)

if "%1"=="flash-com14" (
    echo Flashing to COM14 (GPIO38 - Espressif)...
    call idf.py -p COM14 flash
    goto :eof
)

if "%1"=="flash-com11" (
    echo Flashing to COM11 (GPIO48 - EWeAct)...
    call idf.py -p COM11 flash
    goto :eof
)

if "%1"=="flash-both" (
    echo Flashing to COM14 (GPIO38 - Espressif)...
    call idf.py -p COM14 flash
    if errorlevel 1 (
        echo Failed to flash COM14
        goto :eof
    )
    echo.
    echo Flashing to COM11 (GPIO48 - EWeAct)...
    call idf.py -p COM11 flash
    goto :eof
)

if "%1"=="monitor-com14" (
    echo Monitoring COM14 (GPIO38 - Espressif)...
    call idf.py -p COM14 monitor
    goto :eof
)

if "%1"=="monitor-com11" (
    echo Monitoring COM11 (GPIO48 - EWeAct)...
    call idf.py -p COM11 monitor
    goto :eof
)

if "%1"=="full" (
    echo.
    echo ========================================
    echo Full Build: Clean + Build + Flash COM14 + Monitor
    echo ========================================
    echo.
    call idf.py fullclean
    if errorlevel 1 goto :eof
    call idf.py build
    if errorlevel 1 goto :eof
    call idf.py -p COM14 flash
    if errorlevel 1 goto :eof
    call idf.py -p COM14 monitor
    goto :eof
)

if "%1"=="full-both" (
    echo.
    echo ========================================
    echo Full Build: Clean + Build + Flash BOTH + Monitor COM14
    echo ========================================
    echo.
    call idf.py fullclean
    if errorlevel 1 goto :eof
    call idf.py build
    if errorlevel 1 goto :eof
    call idf.py -p COM14 flash
    if errorlevel 1 goto :eof
    call idf.py -p COM11 flash
    if errorlevel 1 goto :eof
    call idf.py -p COM14 monitor
    goto :eof
)

echo Unknown command: %1
echo Run "build.bat" without arguments for help

