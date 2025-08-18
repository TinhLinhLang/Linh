@echo off
setlocal enabledelayedexpansion

echo +-------------------------------+
echo   Build project in Debug mode
echo +-------------------------------+
echo.

REM Set build directory
set BUILD_DIR=build

REM Save current directory
set ROOT_DIR=%CD%

REM Create build directory if it doesn't exist
if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

REM Change to build directory
cd "%BUILD_DIR%"

REM Configure CMake with Visual Studio Community generator
cmake .. -G "Visual Studio 17 2022" -A x64
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed!
    cd "%ROOT_DIR%"
    exit /b %errorlevel%
)

REM Build project in Debug mode
cmake --build . --config Debug
if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    cd "%ROOT_DIR%"
    exit /b %errorlevel%
)

REM Return to root directory
cd "%ROOT_DIR%"

echo +-------------------------------+
echo   Build completed successfully!
echo +-------------------------------+
echo.

REM Automatically run Debug executable if build succeeded
set EXE_PATH=%BUILD_DIR%\Debug\LinhApp.exe
if exist "%EXE_PATH%" (
    echo Running Debug build: %EXE_PATH%
    "%EXE_PATH%"
) else (
    echo [WARNING] Debug executable not found: %EXE_PATH%
)

endlocal