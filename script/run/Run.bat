@echo off
setlocal EnableDelayedExpansion

REM Check if input file is provided
if "%~1"=="" (
    echo Usage: %0 ^<input_file^>
    echo Example: %0 Test\comprehensive_test.li
    exit /b 1
)

set INPUT_FILE=%~1
set LOG_FILE=run.log

REM Check if input file exists
if not exist "%INPUT_FILE%" (
    echo [ERROR] Input file not found: %INPUT_FILE%
    exit /b 1
)

REM Find LinhApp.exe
set EXE_PATH=build\Debug\LinhApp.exe
if not exist "%EXE_PATH%" (
    set EXE_PATH=build\Release\LinhApp.exe
    if not exist "%EXE_PATH%" (
        echo [ERROR] LinhApp.exe not found in Debug or Release folder!
        exit /b 1
    )
)

echo Running: %EXE_PATH% %INPUT_FILE%
echo Saving output to: %LOG_FILE%
echo.
"%EXE_PATH%" "%INPUT_FILE%" > "%LOG_FILE%" 2>&1
set "EXIT_CODE=!ERRORLEVEL!"
if not "!EXIT_CODE!"=="0" (
    echo [ERROR] LinhApp exited with code !EXIT_CODE!. Check %LOG_FILE% for details.
    echo. >>"%LOG_FILE%"
    echo [ERROR] ScriptRun : LinhApp exited with code !EXIT_CODE!. >>"%LOG_FILE%"
    exit /b !EXIT_CODE!
)

echo.
echo === Execution completed. Check %LOG_FILE% for output ===
endlocal