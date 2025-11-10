@echo off
REM Phase 5: USB Mode Switching - Build and Flash Script
REM Author: Abdul Raheem Ansari
REM Date: November 2025

echo.
echo ========================================
echo Phase 5: USB Mode Switching
echo Build and Flash Script
echo ========================================
echo.

REM ESP-IDF Installation Path
set "IDF_PATH=C:\Users\Abdul\esp-idf-v5.5.1"
set "IDF_TOOLS=%IDF_PATH%\export.bat"

REM Check if ESP-IDF exists
if not exist "%IDF_TOOLS%" (
    echo ERROR: ESP-IDF not found at %IDF_PATH%
    echo Please update the IDF_PATH variable in this script
    echo.
    pause
    exit /b 1
)

echo [1/4] Initializing ESP-IDF environment...
call "%IDF_TOOLS%"
if errorlevel 1 (
    echo ERROR: Failed to initialize ESP-IDF
    pause
    exit /b 1
)
echo.

echo [2/4] Building firmware...
echo.
idf.py build
if errorlevel 1 (
    echo.
    echo ========================================
    echo BUILD FAILED!
    echo ========================================
    echo.
    echo Please check the error messages above.
    echo.
    pause
    exit /b 1
)
echo.

echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo.
echo Firmware built successfully!
echo Binary location: build\esp32s3_michael_usb_host.bin
echo.
echo [3/4] Do you want to flash to boards?
echo.
echo Options:
echo   1 - Flash to COM14 (GPIO38 - Espressif)
echo   2 - Flash to COM11 (GPIO48 - EWeAct)
echo   3 - Flash to BOTH boards
echo   4 - Skip flashing (just build)
echo.
set /p FLASH_CHOICE="Enter choice (1-4): "

if "%FLASH_CHOICE%"=="1" goto flash_com14
if "%FLASH_CHOICE%"=="2" goto flash_com11
if "%FLASH_CHOICE%"=="3" goto flash_both
if "%FLASH_CHOICE%"=="4" goto done
echo Invalid choice, skipping flash
goto done

:flash_com14
echo.
echo [3/4] Flashing to COM14 (GPIO38 - Espressif)...
idf.py -p COM14 flash
if errorlevel 1 (
    echo Flash failed!
    pause
    exit /b 1
)
goto monitor_choice

:flash_com11
echo.
echo [3/4] Flashing to COM11 (GPIO48 - EWeAct)...
idf.py -p COM11 flash
if errorlevel 1 (
    echo Flash failed!
    pause
    exit /b 1
)
goto monitor_choice

:flash_both
echo.
echo [3/4] Flashing to COM14 (GPIO38 - Espressif)...
idf.py -p COM14 flash
if errorlevel 1 (
    echo Flash to COM14 failed!
    pause
    exit /b 1
)
echo.
echo Flashing to COM11 (GPIO48 - EWeAct)...
idf.py -p COM11 flash
if errorlevel 1 (
    echo Flash to COM11 failed!
    pause
    exit /b 1
)
goto monitor_choice

:monitor_choice
echo.
echo ========================================
echo FLASH SUCCESSFUL!
echo ========================================
echo.
echo [4/4] Do you want to monitor serial output?
echo.
echo Options:
echo   1 - Monitor COM14
echo   2 - Monitor COM11
echo   3 - Skip monitoring
echo.
set /p MONITOR_CHOICE="Enter choice (1-3): "

if "%MONITOR_CHOICE%"=="1" goto monitor_com14
if "%MONITOR_CHOICE%"=="2" goto monitor_com11
if "%MONITOR_CHOICE%"=="3" goto done
echo Invalid choice, skipping monitor
goto done

:monitor_com14
echo.
echo Monitoring COM14 (Press Ctrl+] to exit)...
echo.
idf.py -p COM14 monitor
goto done

:monitor_com11
echo.
echo Monitoring COM11 (Press Ctrl+] to exit)...
echo.
idf.py -p COM11 monitor
goto done

:done
echo.
echo ========================================
echo Phase 5 Build Complete!
echo ========================================
echo.
echo Next Steps:
echo   1. Test triple-press mode switching (press BOOT button 3x)
echo   2. Verify LED colors:
echo      - Green slow blink = USB Host mode (default)
echo      - Orange fast blink = Mode switching in progress
echo      - Blue slow blink = USB Device mode
echo   3. Test USB Host workflow (insert USB drive)
echo   4. Test mode switch denial (try switching during file copy)
echo.
echo Documentation:
echo   - PHASE5_IMPLEMENTATION_COMPLETE.md (implementation details)
echo   - PHASE5_USB_MODE_SWITCHING_RECOMMENDATION.md (technical analysis)
echo.
pause

