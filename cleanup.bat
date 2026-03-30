@echo off
echo ========================================
echo Safe Project Cleanup Script
echo ========================================
echo.

REM Change to script directory
cd /d "%~dp0"

echo [1/3] Deleting 'nul' file...
if exist "nul" (
    del /F /Q "nul" 2>nul
    echo     - Deleted: nul
) else (
    echo     - Not found: nul
)

echo.
echo [2/3] Deleting .idea folder (IDE config)...
if exist ".idea" (
    rmdir /S /Q ".idea" 2>nul
    echo     - Deleted: .idea\
) else (
    echo     - Not found: .idea\
)

echo.
echo [3/3] Deleting debug-build folder (build artifacts)...
if exist "debug-build" (
    rmdir /S /Q "debug-build" 2>nul
    echo     - Deleted: debug-build\
) else (
    echo     - Not found: debug-build\
)

echo.
echo ========================================
echo Cleanup Complete!
echo ========================================
echo.
echo You can now rebuild the project with:
echo   mkdir debug-build
echo   cd debug-build
echo   cmake .. -G Ninja
echo   cmake --build .
echo.
pause
