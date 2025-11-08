@echo off
echo.
echo ========================================
echo Phase 1: Building and Flashing COM14
echo ========================================
echo.

REM Set IDF_PATH
set "IDF_PATH=C:\Users\Abdul\esp-idf-v5.5.1"
set "IDF_TOOLS_PATH=C:\Users\Abdul\.espressif"

REM Navigate to project directory
cd /d "%~dp0"

echo [1/5] Setting up environment variables...

REM Add ESP32 tools to PATH manually
set "PATH=%IDF_TOOLS_PATH%\tools\xtensa-esp-elf\esp-14.2.0_20241119\xtensa-esp-elf\bin;%PATH%"
set "PATH=%IDF_TOOLS_PATH%\tools\riscv32-esp-elf\esp-14.2.0_20241119\riscv32-esp-elf\bin;%PATH%"
set "PATH=%IDF_TOOLS_PATH%\tools\cmake\3.30.2\bin;%PATH%"
set "PATH=%IDF_TOOLS_PATH%\tools\ninja\1.12.1;%PATH%"
set "PATH=%IDF_TOOLS_PATH%\tools\idf-exe\1.0.3;%PATH%"
set "PATH=%IDF_TOOLS_PATH%\tools\ccache\4.11.2\ccache-4.11.2-windows-x86_64;%PATH%"
set "PATH=%IDF_TOOLS_PATH%\python_env\idf5.5_py3.11_env\Scripts;%PATH%"
set "PATH=%IDF_PATH%\tools;%PATH%"

echo IDF_PATH=%IDF_PATH%
echo.

echo [2/5] Verifying compiler...
where xtensa-esp32s3-elf-gcc
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Compiler not found in PATH!
    pause
    exit /b 1
)

echo.
echo [3/5] Cleaning old build...
if exist build rmdir /s /q build

echo.
echo [4/5] Building firmware...
python %IDF_PATH%\tools\idf.py build

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ========================================
    echo BUILD FAILED!
    echo ========================================
    pause
    exit /b 1
)

echo.
echo [5/5] Flashing to COM14...
python %IDF_PATH%\tools\idf.py -p COM14 flash

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ========================================
    echo FLASH FAILED!
    echo ========================================
    echo.
    echo Make sure:
    echo   - COM14 is connected
    echo   - PuTTY is closed on COM14
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo SUCCESS! Firmware flashed to COM14
echo ========================================
echo.
echo Next: Open PuTTY on COM14 (115200 baud)
echo.
echo You should see:
echo   - Board: Espressif DevKitC-1-N8R8
echo   - LED GPIO: 38
echo   - LED animation task started
echo   - LED state test sequence
echo.
echo Physical LED should show:
echo   1. Green slow blink (IDLE - 5 sec)
echo   2. Cyan fast blink (PREPARE - 3 sec)
echo   3. Yellow blink (COPY - 3 sec)
echo   4. Magenta blink (SYNC - 3 sec)
echo   5. Green solid (SUCCESS - 3 sec)
echo   6. Red fast blink (ERROR - 3 sec)
echo   7. Back to green slow blink (IDLE)
echo.
pause

