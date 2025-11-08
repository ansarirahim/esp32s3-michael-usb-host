@echo off
setlocal enabledelayedexpansion

echo.
echo ========================================
echo Phase 1: Ultimate Build Script
echo ========================================
echo.

REM Change to project directory
cd /d "%~dp0"

REM Set IDF_PATH
set "IDF_PATH=C:\Users\Abdul\esp-idf-v5.5.1"

echo [1/4] Running ESP-IDF export.bat...
call "%IDF_PATH%\export.bat" >nul 2>&1

echo [2/4] Running fullclean...
"%IDF_PATH%\tools\idf.py" fullclean

echo.
echo [3/4] Building firmware (this may take 2-3 minutes)...
"%IDF_PATH%\tools\idf.py" build

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ========================================
    echo BUILD FAILED!
    echo ========================================
    echo.
    echo Please check the error above.
    pause
    exit /b 1
)

echo.
echo [4/4] Flashing to COM14...
"%IDF_PATH%\tools\idf.py" -p COM14 flash

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ========================================
    echo FLASH FAILED!
    echo ========================================
    echo.
    echo Make sure COM14 is available and PuTTY is closed.
    pause
    exit /b 1
)

echo.
echo ========================================
echo SUCCESS! Phase 1 firmware flashed!
echo ========================================
echo.
echo Now check PuTTY on COM14 to see:
echo   - LED initialization
echo   - LED state test sequence
echo   - Physical LED blinking in colors
echo.
pause

