@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem =====================================================================
rem build.bat - build pz-int (Project Zomboid internal DLL)
rem
rem 1. builds pz-int\pz-int.vcxproj -> pz-int\release\pz-int.dll
rem
rem usage: build.bat [clean|rebuild|build]
rem =====================================================================

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "PROJ=%ROOT%\pz-int.vcxproj"
set "OUT=%ROOT%\release\pz-int.dll"

set "TARGET=%~1"
if "%TARGET%"=="" set "TARGET=build"

rem ---- locate MSBuild --------------------------------------------------

set "MSBUILD="
for %%P in (
    "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
    "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
) do (
    if exist %%~P (
        set "MSBUILD=%%~P"
        goto :found_msbuild
    )
)

set "VSWHERE=C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe"`) do (
        set "MSBUILD=%%I"
    )
)

:found_msbuild
if not defined MSBUILD (
    echo [build] ERROR: MSBuild.exe not found.
    exit /b 1
)

echo [build] MSBuild: %MSBUILD%
echo [build] target : %TARGET%
echo.

if /i "%TARGET%"=="clean" goto :do_clean
if /i "%TARGET%"=="rebuild" goto :do_rebuild
goto :build_incremental

:do_clean
if exist "%ROOT%\release" rmdir /s /q "%ROOT%\release"
if exist "%ROOT%\intermediates" rmdir /s /q "%ROOT%\intermediates"
echo [build] clean done. rebuilding...
set "BUILD_VERB=Rebuild"
goto :do_build

:do_rebuild
set "BUILD_VERB=Rebuild"
goto :do_build

:build_incremental
set "BUILD_VERB=Build"
goto :do_build

:do_build
if not exist "%PROJ%" (
    echo [build] ERROR: %PROJ% not found.
    exit /b 1
)

"%MSBUILD%" "%PROJ%" ^
    /p:Configuration=Release ^
    /p:Platform=x64 ^
    /t:%BUILD_VERB% ^
    /m ^
    /nologo ^
    /v:minimal

if errorlevel 1 (
    echo.
    echo [build] ERROR: build failed.
    exit /b 1
)

if not exist "%OUT%" (
    echo [build] ERROR: DLL not produced at %OUT%
    exit /b 1
)

echo.
echo [build] ==============================================================
echo [build] success: %OUT%
echo [build] ==============================================================
for %%F in ("%OUT%") do echo [build]   %%~zF bytes
echo [build] inject with Process Hacker into ProjectZomboid64.exe
exit /b 0
