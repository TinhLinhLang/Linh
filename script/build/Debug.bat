@echo off
setlocal

echo +-------------------------------+
echo   Build project in Debug mode
echo +-------------------------------+
echo(
REM Set build directory
set BUILD_DIR=build

REM Save current directory
set ROOT_DIR=%CD%

REM Create build directory if it doesn't exist
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

REM Change to build directory
cd "%BUILD_DIR%"

REM Ensure the CMake generator matches Visual Studio 17 2022
set "TARGET_GENERATOR=Visual Studio 17 2022"
set "CACHE_GEN="
if exist CMakeCache.txt for /f "tokens=2 delims==" %%G in ('findstr /B /C:"CMAKE_GENERATOR:INTERNAL=" CMakeCache.txt') do set "CACHE_GEN=%%G"
if not defined CACHE_GEN goto :gen_checked
if /I "%CACHE_GEN%"=="%TARGET_GENERATOR%" goto :gen_checked
echo [INFO] Generator mismatch detected (found: "%CACHE_GEN%", expected: "%TARGET_GENERATOR%")
echo [INFO] Cleaning CMake cache...
if exist CMakeFiles rmdir /s /q CMakeFiles
if exist CMakeCache.txt del /f /q CMakeCache.txt
:gen_checked

REM Configure CMake with Visual Studio Community generator
cmake .. -G "Visual Studio 17 2022" -A x64
if errorlevel 1 goto :conf_error

REM Build project in Debug mode
cmake --build . --config Debug
if errorlevel 1 goto :build_error

REM Return to root directory
cd "%ROOT_DIR%"

echo +-------------------------------+
echo   Build completed successfully!
echo +-------------------------------+
echo(

REM Automatically run Debug executable if build succeeded
set EXE_PATH=%BUILD_DIR%\Debug\LinhApp.exe
if exist "%EXE_PATH%" goto :run_debug
echo [WARNING] Debug executable not found: %EXE_PATH%
goto :eof_dbg
:run_debug
echo Running Debug build: %EXE_PATH%
"%EXE_PATH%"
:eof_dbg
goto :eof

:conf_error
echo [ERROR] CMake configuration failed!
cd "%ROOT_DIR%"
exit /b 1

:build_error
echo [ERROR] Build failed!
cd "%ROOT_DIR%"
exit /b 1

:eof
endlocal