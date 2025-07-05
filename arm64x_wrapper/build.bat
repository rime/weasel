@echo off
setlocal enabledelayedexpansion

set "min_sdk_version=10.0.22000.0"
set "sdk_base=C:\Program Files (x86)\Windows Kits\10\Lib\"
set "SDKVER_ARM64EC="
pushd %sdk_base%
for /f "delims=" %%i in ('ls ^| "grep.exe" -E "^10\.[0-9]+\.[0-9]+\.[0-9]+$" ^| "sort.exe"') do (
    set "current_sdk=%%i"
    if "!current_sdk!" geq "%min_sdk_version%" (
        set "SDKVER_ARM64EC=!current_sdk!"
        echo Found suitable SDK version: !SDKVER_ARM64EC!
        goto found_sdk
    )
)
:found_sdk

popd
if not defined SDKVER_ARM64EC (
    echo No suitable SDK found, defaulting to %min_sdk_version%
    set "SDKVER_ARM64EC=%min_sdk_version%"
)

echo Using SDK version: %SDKVER_ARM64EC%
if defined SDKVER_ARM64EC (
    set "build_sdk_option_wrapper=-winsdk=%SDKVER_ARM64EC%"
    rem set "LIB=C:\Program Files (x86)\Windows Kits\10\Lib\%SDKVER_ARM64EC%\um\arm64;!LIB!;"
) else (
    echo No suitable SDK found, exiting.
    exit /b 1
)

echo build_sdk_option_wrapper set to: !build_sdk_option_wrapper!

call :find_toolchain
if errorlevel 1 exit /b 1

REM Build dummy object files
cl.exe /c /Fo:dummy.o dummy.c || exit /b 1
cl.exe /c /arm64EC /Fo:dummy_x64.o dummy.c || exit /b 1

REM Build weasel.dll wrapper
link.exe /lib /machine:x64 /def:WeaselTSF_x64.def /out:WeaselTSF_x64.lib /ignore:4104
link.exe /lib /machine:arm64 /def:WeaselTSF_arm64.def /out:WeaselTSF_arm64.lib /ignore:4104
rem link.exe /dll /noentry /machine:arm64x /defArm64Native:WeaselTSF_arm64.def /def:WeaselTSF_x64.def ^
rem   /out:weaselARM64X.dll dummy.o dummy_x64.o WeaselTSF_x64.lib WeaselTSF_arm64.lib /ignore:4104

REM Build weasel.ime wrapper
link.exe /lib /machine:x64 /def:WeaselIME_x64.def /out:WeaselIME_x64.lib /ignore:4104
link.exe /lib /machine:arm64 /def:WeaselIME_arm64.def /out:WeaselIME_arm64.lib /ignore:4104
rem link.exe /dll /noentry /machine:arm64x /defArm64Native:WeaselIME_arm64.def /def:WeaselIME_x64.def ^
rem   /out:weaselARM64X.ime dummy.o dummy_x64.o WeaselIME_x64.lib WeaselIME_arm64.lib /ignore:4104

endlocal
exit /b

:find_toolchain
set VSWHERE="%PROGRAMFILES%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist %VSWHERE% (
  set VSWHERE="%PROGRAMFILES(X86)%\Microsoft Visual Studio\Installer\vswhere.exe"
)
if not exist %VSWHERE% (
  echo vswhere not found, could not find a working MSVC install.
  exit /b 1
)

for /f "usebackq tokens=*" %%i in (`%VSWHERE% -latest -requires Microsoft.VisualStudio.Component.VC.Tools.ARM64EC -property installationPath`) do (
  set ARM64EC_TOOLCHAIN=%%i
)

if exist "%ARM64EC_TOOLCHAIN%\Common7\Tools\vsdevcmd.bat" (
  call "%ARM64EC_TOOLCHAIN%\Common7\Tools\vsdevcmd.bat" -arch=arm64 -host_arch=x86 !build_sdk_option_wrapper!
  echo.
) else (
  echo ARM64EC capable toolchain not found
  exit /b 1
)

exit /b

