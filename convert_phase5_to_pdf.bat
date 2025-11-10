@echo off
REM Phase 5 Documentation - Convert to PDF (Windows Batch)
REM This script tries multiple conversion methods

echo ========================================
echo Phase 5 Documentation - MD to PDF
echo ========================================
echo.

REM Check if pandoc is installed
where pandoc >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [OK] pandoc found
    echo.
    echo Using PowerShell script...
    powershell -ExecutionPolicy Bypass -File scripts\convert_phase5_to_pdf.ps1
    goto :end
)

REM Check if Python is installed
where python >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [OK] Python found
    echo.
    echo Using Python script...
    python scripts\convert_phase5_to_pdf.py
    goto :end
)

REM Neither pandoc nor Python found
echo [ERROR] Neither pandoc nor Python found!
echo.
echo Please install one of the following:
echo   1. pandoc: https://pandoc.org/installing.html
echo   2. Python: https://www.python.org/downloads/
echo.
pause
exit /b 1

:end
echo.
pause

