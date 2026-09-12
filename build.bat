@echo off
rem Builds minic.exe with MSVC. Finds Visual Studio through vswhere,
rem so any edition (Community, Professional, BuildTools) works.
setlocal

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%VSWHERE%" (
    echo vswhere.exe not found. Is Visual Studio installed?
    exit /b 1
)

rem Via a temp file, to avoid quoting a path that contains spaces
rem inside a for /f backquote command.
set "VSPATH="
set "VSPATHFILE=%TEMP%\minic_vspath.txt"

"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "%VSPATHFILE%"
set /p VSPATH=<"%VSPATHFILE%"
del "%VSPATHFILE%" >nul 2>&1

if not defined VSPATH (
    echo No Visual Studio installation with the C++ toolchain was found.
    exit /b 1
)

echo Using %VSPATH%
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1

cl /nologo /std:c++17 /EHsc /W4 /I src /Fe:minic.exe src/main.cpp src/lexer.cpp
if errorlevel 1 exit /b 1

echo Built minic.exe
