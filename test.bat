@echo off
rem Runs every tests\*.c through minic.exe and compares stdout plus stderr
rem with the matching .expected file. Files named run_*.c are executed with
rem --run; all other tests use the default semantic-check mode.
rem
rem   test.bat          run all tests
rem   test.bat update   rewrite every .expected from the current output,
rem                     then review the change with git diff
setlocal enabledelayedexpansion

cd /d "%~dp0"

rem Full path on purpose: cmd does not search the current directory
rem when NoDefaultCurrentDirectoryInExePath is set.
set "MINIC=%~dp0minic.exe"

if not exist "%MINIC%" (
    echo minic.exe not found. Run build.bat first.
    exit /b 1
)

set "OUT=%TEMP%\minic_test_output.txt"
set /a passed=0
set /a failed=0

for %%f in (tests\*.c) do (
    set "ARGS="
    set "NAME=%%~nf"
    if /i "!NAME:~0,4!"=="run_" set "ARGS=--run"

    "%MINIC%" !ARGS! "%%f" > "!OUT!" 2>&1

    if /i "%~1"=="update" (
        copy /y "!OUT!" "%%~dpnf.expected" >nul
        set /a passed+=1
    ) else (
        fc "%%~dpnf.expected" "!OUT!" >nul 2>&1

        if errorlevel 1 (
            echo FAIL  %%~nxf
            fc "%%~dpnf.expected" "!OUT!"
            set /a failed+=1
        ) else (
            set /a passed+=1
        )
    )
)

del "!OUT!" >nul 2>&1

if /i "%~1"=="update" (
    echo Updated !passed! .expected files. Review them with git diff.
    exit /b 0
)

echo !passed! passed, !failed! failed

if !failed! gtr 0 exit /b 1
exit /b 0
