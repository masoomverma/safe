@echo off
echo ========================================
echo Safe Project Cleanup Script
echo ========================================
echo.

cd /d D:\Work\Project\CPP\Safe

echo [1/5] Deleting 'nul' file...
if exist "nul" (
    del /F /Q "nul" 2>nul
    echo     - Deleted: nul
) else (
    echo     - Not found: nul
)

echo.
echo [2/5] Deleting empty renderer.cpp...
if exist "src\core\renderer.cpp" (
    del /F /Q "src\core\renderer.cpp" 2>nul
    echo     - Deleted: src\core\renderer.cpp
) else (
    echo     - Not found: src\core\renderer.cpp
)

echo.
echo [3/5] Deleting .idea folder (IDE config)...
if exist ".idea" (
    rmdir /S /Q ".idea" 2>nul
    echo     - Deleted: .idea\
) else (
    echo     - Not found: .idea\
)

echo.
echo [4/5] Deleting debug-build folder (build artifacts)...
if exist "debug-build" (
    rmdir /S /Q "debug-build" 2>nul
    echo     - Deleted: debug-build\
) else (
    echo     - Not found: debug-build\
)

echo.
echo [5/5] Deleting core folder (abandoned structure)...
if exist "core" (
    rmdir /S /Q "core" 2>nul
    echo     - Deleted: core\
) else (
    echo     - Not found: core\
)

echo.
echo ========================================
echo Cleanup Complete!
echo ========================================
echo.
echo Remaining project structure:
echo.
dir /B
echo.
echo You can now rebuild the project with:
echo cmake -B build -S . -G Ninja
echo cmake --build build
echo.
pause
