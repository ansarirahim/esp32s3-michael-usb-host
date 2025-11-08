@echo off
REM Quick Build and Flash Script for ESP32-S3 USB Host Automator
REM This script builds the firmware and flashes it to both COM14 and COM11

echo.
echo ========================================
echo ESP32-S3 USB Host Automator
echo Build and Flash to COM14 and COM11
echo ========================================
echo.

REM ESP-IDF Installation Path
set "IDF_PATH=C:\Users\Abdul\esp-idf-v5.5.1"
set "IDF_TOOLS=%IDF_PATH%\export.bat"

REM Check if ESP-IDF exists
if not exist "%IDF_TOOLS%" (
    echo ERROR: ESP-IDF not found at %IDF_PATH%
    echo.
    echo Please check your ESP-IDF installation
    pause
    exit /b 1
)

echo [1/5] Initializing ESP-IDF environment...
call "%IDF_TOOLS%"
if errorlevel 1 (
    echo ERROR: Failed to initialize ESP-IDF
    pause
    exit /b 1
)
echo.

echo [2/5] Building firmware...
idf.py build
if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)
echo.

echo [3/5] Flashing to COM14 (GPIO38 - Espressif)...
idf.py -p COM14 flash
if errorlevel 1 (
    echo.
    echo ERROR: Failed to flash COM14
    echo Make sure the board is connected to COM14
    pause
    exit /b 1
)
echo.

echo [4/5] Flashing to COM11 (GPIO48 - EWeAct)...
idf.py -p COM11 flash
if errorlevel 1 (
    echo.
    echo ERROR: Failed to flash COM11
    echo Make sure the board is connected to COM11
    pause
    exit /b 1
)
echo.

echo [5/5] SUCCESS! Both boards flashed successfully!
echo.
echo ========================================
echo Next Steps:
echo ========================================
echo.
echo To monitor COM14: idf.py -p COM14 monitor
echo To monitor COM11: idf.py -p COM11 monitor
echo.
echo Press Ctrl+] to exit monitor
echo.
pause

