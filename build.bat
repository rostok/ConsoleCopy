@echo off
setlocal EnableExtensions

set "PLATFORM=%~1"
if not "%PLATFORM%"=="Win32" if not "%PLATFORM%"=="x64" (
    echo Usage: build.bat Win32^|x64 [Far source directory]
    exit /b 2
)

if not "%~2"=="" set "FAR_SOURCE_ROOT=%~2"
if not defined FAR_SOURCE_ROOT set "FAR_SOURCE_ROOT=%~dp0..\.."
for %%I in ("%FAR_SOURCE_ROOT%") do set "FAR_SOURCE_ROOT=%%~fI"

if not exist "%FAR_SOURCE_ROOT%\_build\vc\config\common.plugins.props" (
    echo Far source tree not found: "%FAR_SOURCE_ROOT%"
    echo Pass the directory containing _build, far, and plugins.
    exit /b 2
)
if not exist "%FAR_SOURCE_ROOT%\plugins\common\unicode\plugin.hpp" (
    echo Far plugin headers not found in: "%FAR_SOURCE_ROOT%"
    exit /b 2
)

set "MSBUILD="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
        if not defined MSBUILD if exist "%%I\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=%%I\MSBuild\Current\Bin\MSBuild.exe"
    )
)
if not defined MSBUILD (
    for /f "delims=" %%I in ('where MSBuild.exe 2^>nul') do if not defined MSBUILD set "MSBUILD=%%I"
)
if not defined MSBUILD (
    echo MSBuild.exe not found. Install Visual Studio with C++ build tools.
    exit /b 2
)

echo Far sources: "%FAR_SOURCE_ROOT%"
echo Platform: %PLATFORM%
"%MSBUILD%" "%~dp0ConsoleCopy.vcxproj" /m /p:Configuration=Release /p:Platform=%PLATFORM% "/p:FarSourceRoot=%FAR_SOURCE_ROOT%" /v:minimal
if errorlevel 1 exit /b %errorlevel%

set "OUTPUT=%FAR_SOURCE_ROOT%\_build\vc\_output\product\Release.%PLATFORM%\Plugins\ConsoleCopy\ConsoleCopy.dll"
if not exist "%OUTPUT%" (
    echo Build reported success, but DLL was not found: "%OUTPUT%"
    exit /b 1
)
echo Built: "%OUTPUT%"
exit /b 0
