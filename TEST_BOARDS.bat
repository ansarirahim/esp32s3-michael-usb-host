@echo off
REM Test Both Boards - ESP32-S3 USB Host Automator
REM This script monitors both COM ports to verify board detection

echo.
echo ========================================
echo ESP32-S3 USB Host Automator
echo Testing Board Detection
echo ========================================
echo.

REM ESP-IDF Installation Path
set "IDF_PATH=C:\Users\Abdul\esp-idf-v5.5.1"
set "IDF_TOOLS=%IDF_PATH%\export.bat"

echo Initializing ESP-IDF environment...
call "%IDF_TOOLS%" >nul 2>&1

echo.
echo ========================================
echo Step 1: Testing COM14 (Espressif)
echo ========================================
echo.
echo Press RESET button on COM14 board now...
echo Waiting 3 seconds...
timeout /t 3 /nobreak >nul

echo.
echo Monitoring COM14 for 10 seconds...
echo Look for MAC address and GPIO detection...
echo.

start /b cmd /c "idf.py -p COM14 monitor > com14_output.txt 2>&1"
timeout /t 10 /nobreak >nul

echo.
echo ========================================
echo COM14 Output:
echo ========================================
type com14_output.txt | findstr /i "MAC GPIO Board app board_pins"
echo.

echo.
echo ========================================
echo Step 2: Testing COM11 (EWeAct)
echo ========================================
echo.
echo Press RESET button on COM11 board now...
echo Waiting 3 seconds...
timeout /t 3 /nobreak >nul

echo.
echo Monitoring COM11 for 10 seconds...
echo Look for MAC address and GPIO detection...
echo.

start /b cmd /c "idf.py -p COM11 monitor > com11_output.txt 2>&1"
timeout /t 10 /nobreak >nul

echo.
echo ========================================
echo COM11 Output:
echo ========================================
type com11_output.txt | findstr /i "MAC GPIO Board app board_pins"
echo.

echo.
echo ========================================
echo Test Complete!
echo ========================================
echo.
echo Full logs saved to:
echo   - com14_output.txt
echo   - com11_output.txt
echo.
echo Expected output for each board:
echo   - Chip MAC address: XX:XX:XX:XX:XX:XX
echo   - Board: [Board Name]
echo   - LED GPIO: 38 or 48
echo.
pause

