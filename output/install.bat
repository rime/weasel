@echo off

rem argument 1: [ /s | /t ] register ime as zh_CN | zh_TW keyboard layout
set WEASEL_IME=weasel
if /i "%1" == "/t" set WEASEL_IME=weaselt

cd "%~dp0"

echo stopping service from an older version.
call stop_service.bat

echo configuring preset input schemas...
weaseldeployer.exe /install

echo registering Weasel IME to your system.

wscript check_windows_version.js
if errorlevel 2 goto win7_x64_install
if errorlevel 1 goto xp_install

:win7_install
wscript elevate.js rundll32 "%CD%\%WEASEL_IME%.ime" install
if %ERRORLEVEL% EQU 0 goto success

:win7_x64_install
wscript elevate.js rundll32 "%CD%\%WEASEL_IME%x64.ime" install
if %ERRORLEVEL% EQU 0 goto success

:xp_install
rundll32 "%CD%\%WEASEL_IME%.ime" install
if %ERRORLEVEL% EQU 0 goto success

:success

rem start notepad README.txt

:exit
