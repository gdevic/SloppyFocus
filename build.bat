@echo off
setlocal enableextensions

rem ---------------------------------------------------------------------------
rem SloppyFocus build
rem Locates VS2022 via vswhere, sets up cl/rc env, builds SloppyFocus.exe (x64).
rem Final exe lands in the project root; intermediates in build\.
rem ---------------------------------------------------------------------------

set "APP=SloppyFocus"
set "OUT_EXE=%APP%.exe"
set "BUILD_DIR=build"
set "SRC_DIR=src"
set "RES_DIR=res"
set "RC_FILE=%RES_DIR%\app.rc"
set "MANIFEST=%RES_DIR%\app.manifest"
set "SOURCES=%SRC_DIR%\main.cpp"
set "LIBS=user32.lib gdi32.lib advapi32.lib shell32.lib comctl32.lib"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo ERROR: vswhere.exe not found. Install Visual Studio 2022 with C++ workload.
    exit /b 1
)

set "VSPATH="
"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "%TEMP%\%APP%_vspath.txt"
for /f "usebackq delims=" %%i in ("%TEMP%\%APP%_vspath.txt") do set "VSPATH=%%i"
del /q "%TEMP%\%APP%_vspath.txt" 2>nul
if not defined VSPATH (
    echo ERROR: VS2022 with C++ tools not found.
    exit /b 1
)

rem vcvarsall.bat calls 'vswhere' bare; ensure its dir is on PATH so its lookup
rem succeeds (otherwise it emits a noisy "not recognized" warning).
for %%a in ("%VSWHERE%") do set "PATH=%%~dpa;%PATH%"

call "%VSPATH%\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul
if errorlevel 1 (
    echo ERROR: vcvarsall.bat x64 failed.
    exit /b 1
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

rc /nologo /I"%SRC_DIR%" /I"%RES_DIR%" /fo "%BUILD_DIR%\app.res" "%RC_FILE%"
if errorlevel 1 (
    echo ERROR: rc failed.
    exit /b 1
)

cl /nologo /std:c++17 /W4 /WX /permissive- /utf-8 /O1 /Os /GS- /GR- /EHsc /MT ^
   /DUNICODE /D_UNICODE /DWIN32_LEAN_AND_MEAN /DNOMINMAX ^
   /I"%SRC_DIR%" /I"%RES_DIR%" ^
   /Fo"%BUILD_DIR%\\" /Fd"%BUILD_DIR%\%APP%.pdb" ^
   /Fe"%OUT_EXE%" ^
   %SOURCES% ^
   "%BUILD_DIR%\app.res" ^
   /link /SUBSYSTEM:WINDOWS ^
         /MANIFEST:EMBED /MANIFESTINPUT:"%MANIFEST%" ^
         /OPT:REF /OPT:ICF ^
         /MERGE:.rdata=.text ^
         %LIBS%
if errorlevel 1 (
    echo ERROR: cl/link failed.
    exit /b 1
)

echo.
for %%I in (%OUT_EXE%) do echo Built %%~fI  (%%~zI bytes)
exit /b 0
