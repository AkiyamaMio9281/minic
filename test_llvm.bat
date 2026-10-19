@echo off
rem Differential test. Every tests\run_*.c is compiled through the LLVM
rem backend and its output compared against the same .expected file the VM
rem is held to, so the two backends have to agree byte for byte.
rem
rem Each test runs at -O0 and -O2. A difference between the two levels
rem almost always means the generated IR has undefined behavior in it.
rem
rem   test_llvm.bat     run every run_* test at both levels
setlocal enabledelayedexpansion

cd /d "%~dp0"

set "MINIC=%~dp0minic.exe"

if not exist "%MINIC%" (
    echo minic.exe not found. Run build.bat first.
    exit /b 1
)

set "CLANG=C:\Program Files\LLVM\bin\clang.exe"
if not exist "%CLANG%" set "CLANG=clang"

rem The IR is written through cmd's redirection on purpose. PowerShell's >
rem prepends a BOM and clang then rejects the file at line 1, column 1.
set "LL=%TEMP%\minic_llvm.ll"
set "EXE=%TEMP%\minic_llvm_test.exe"
set "OUT=%TEMP%\minic_llvm_output.txt"

set /a passed=0
set /a failed=0

for %%f in (tests\run_*.c) do (
    "%MINIC%" --emit-llvm "%%f" > "!LL!"

    if errorlevel 1 (
        echo FAIL  %%~nxf  minic --emit-llvm failed
        set /a failed+=1
    ) else (
        for %%o in (-O0 -O2) do (
            "%CLANG%" %%o -Wno-override-module "!LL!" runtime\minic_rt.c -o "!EXE!" > "!OUT!" 2>&1

            if errorlevel 1 (
                echo FAIL  %%~nxf  [%%o]  clang rejected the IR
                type "!OUT!"
                set /a failed+=1
            ) else (
                "!EXE!" > "!OUT!" 2>&1
                fc "%%~dpnf.expected" "!OUT!" >nul 2>&1

                if errorlevel 1 (
                    echo FAIL  %%~nxf  [%%o]
                    fc "%%~dpnf.expected" "!OUT!"
                    set /a failed+=1
                ) else (
                    set /a passed+=1
                )
            )
        )
    )
)

del "!LL!" "!EXE!" "!OUT!" >nul 2>&1

echo !passed! passed, !failed! failed

if !failed! gtr 0 exit /b 1
exit /b 0
