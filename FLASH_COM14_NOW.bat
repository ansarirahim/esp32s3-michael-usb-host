@echo off
echo.
echo ========================================
echo Phase 1: Building and Flashing COM14
echo ========================================
echo.

REM Set IDF_PATH
set "IDF_PATH=C:\Users\Abdul\esp-idf-v5.5.1"

REM Navigate to project directory
cd /d "%~dp0"

echo [1/4] Setting IDF_PATH...
echo IDF_PATH=%IDF_PATH%

echo.
echo [2/4] Initializing ESP-IDF environment...
call "%IDF_PATH%\export.bat"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ========================================
    echo ENVIRONMENT SETUP FAILED!
    echo ========================================
    pause
    exit /b 1
)

echo.
echo [3/4] Building firmware...
idf.py build

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ========================================
    echo BUILD FAILED!
    echo ========================================
    echo.
    echo Check the error messages above.
    echo Common issues:
    echo   - Compiler not found: Run install.bat again
    echo   - CMake errors: Delete build folder and retry
    echo.
    pause
    exit /b 1
)

echo.
echo [4/4] Flashing to COM14...
idf.py -p COM14 flash

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ========================================
    echo FLASH FAILED!
    echo ========================================
    echo.
    echo Common issues:
    echo   - COM14 not found: Check device manager
    echo   - Permission denied: Close PuTTY/monitor first
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo SUCCESS! Firmware flashed to COM14
echo ========================================
echo.
echo Next: Check PuTTY on COM14 for LED test
echo You should see:
echo   - Board detection with MAC address
echo   - LED initialization on GPIO 38
echo   - LED animation task started
echo   - LED state test sequence
echo   - Physical LED blinking in different colors:
echo     * Green slow blink (IDLE)
echo     * Cyan fast blink (PREPARE)
echo     * Yellow blink (COPY)
echo     * Magenta blink (SYNC)
echo     * Green solid (SUCCESS)
echo     * Red fast blink (ERROR)
echo.
pause

