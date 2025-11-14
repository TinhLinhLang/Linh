@echo off
setlocal

echo +-------------------------------+
echo   Build project in Debug mode
echo +-------------------------------+
echo(
REM Detect number of CPU cores for parallel build
for /f "skip=2 tokens=2 delims== " %%A in ('wmic cpu get NumberOfLogicalProcessors /value') do set NUM_CORES=%%A
if not defined NUM_CORES set NUM_CORES=4

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

echo. > "%ROOT_DIR%\build.log"

REM Configure CMake with Visual Studio Community generator
echo [INFO] Configuring CMake...
echo [INFO] Configuring CMake at %date% %time% > "%ROOT_DIR%\build.log"
cmake .. -G "Visual Studio 17 2022" -A x64 > "%TEMP%\cmake_config.tmp" 2>&1
set CMAKE_CONFIG_RESULT=%errorlevel%
type "%TEMP%\cmake_config.tmp"
type "%TEMP%\cmake_config.tmp" >> "%ROOT_DIR%\build.log"
del "%TEMP%\cmake_config.tmp" 2>nul
if %CMAKE_CONFIG_RESULT% neq 0 goto :conf_error

REM Build project in Debug mode
echo [INFO] Building project in Debug mode...
echo [INFO] Building project in Debug mode at %date% %time% >> "%ROOT_DIR%\build.log"
cmake --build . --config Debug -- /m:%NUM_CORES% > "%TEMP%\cmake_build.tmp" 2>&1
set CMAKE_BUILD_RESULT=%errorlevel%
type "%TEMP%\cmake_build.tmp"
type "%TEMP%\cmake_build.tmp" >> "%ROOT_DIR%\build.log"
del "%TEMP%\cmake_build.tmp" 2>nul
if %CMAKE_BUILD_RESULT% neq 0 goto :build_error

REM Return to root directory
cd "%ROOT_DIR%"

echo +-------------------------------+
echo   Build completed successfully!
echo +-------------------------------+
echo [SUCCESS] Build completed successfully at %date% %time% >> "%ROOT_DIR%\build.log"
echo(

goto :eof

:conf_error
echo [ERROR] CMake configuration failed!
echo [ERROR] CMake configuration failed at %date% %time% >> "%ROOT_DIR%\build.log"
cd "%ROOT_DIR%"
exit /b 1

:build_error
echo [ERROR] Build failed!
echo [ERROR] Build failed at %date% %time% >> "%ROOT_DIR%\build.log"
cd "%ROOT_DIR%"
exit /b 1

:eof
endlocal