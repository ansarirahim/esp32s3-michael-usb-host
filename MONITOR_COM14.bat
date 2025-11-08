@echo off
REM Monitor COM14 - ESP32-S3 USB Host Automator

echo.
echo ========================================
echo Monitoring COM14 (GPIO38 - Espressif)
echo ========================================
echo.
echo Press Ctrl+] to exit monitor
echo.

REM ESP-IDF Installation Path
set "IDF_PATH=C:\Users\Abdul\esp-idf-v5.5.1"
set "IDF_TOOLS=%IDF_PATH%\export.bat"

REM Initialize ESP-IDF environment
call "%IDF_TOOLS%" >nul 2>&1

REM Monitor COM14
idf.py -p COM14 monitor

