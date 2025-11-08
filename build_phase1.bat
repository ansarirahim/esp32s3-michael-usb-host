@echo off
REM Phase 1 Build and Flash - COM14 Only

echo.
echo ========================================
echo Phase 1: LED Control - Build and Flash
echo ========================================
echo.

set "IDF_PATH=C:\Users\Abdul\esp-idf-v5.5.1"
set "IDF_TOOLS=%IDF_PATH%\export.bat"

echo [1/3] Initializing ESP-IDF...
call "%IDF_TOOLS%"

echo.
echo [2/3] Building firmware...
idf.py build

echo.
echo [3/3] Flashing to COM14...
idf.py -p COM14 flash

echo.
echo ========================================
echo Build and Flash Complete!
echo ========================================
echo.
echo Next: Monitor COM14 to see LED test sequence
echo Command: idf.py -p COM14 monitor
echo.
pause

