@echo off
echo.
echo ========================================
echo Monitoring COM14 - Press Ctrl+] to exit
echo ========================================
echo.

set "IDF_PATH=C:\Users\Abdul\esp-idf-v5.5.1"

call "%IDF_PATH%\export.bat" >nul 2>&1

"%IDF_PATH%\tools\idf.py" -p COM14 monitor

