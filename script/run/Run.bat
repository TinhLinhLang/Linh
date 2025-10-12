@echo off
setlocal

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
if exist "%EXE_PATH%" (
    echo Running: %EXE_PATH% %INPUT_FILE%
    echo Saving output to: %LOG_FILE%
    echo.
    "%EXE_PATH%" "%INPUT_FILE%" > "%LOG_FILE%" 2>&1
    echo.
    echo === Execution completed. Check %LOG_FILE% for output ===
) else (
    set EXE_PATH=build\Release\LinhApp.exe
    if exist "%EXE_PATH%" (
        echo Running: %EXE_PATH% %INPUT_FILE%
        echo Saving output to: %LOG_FILE%
        echo.
        "%EXE_PATH%" "%INPUT_FILE%" > "%LOG_FILE%" 2>&1
        echo.
        echo === Execution completed. Check %LOG_FILE% for output ===
    ) else (
        echo [ERROR] LinhApp.exe not found in Debug or Release folder!
        exit /b 1
    )
)
endlocal
