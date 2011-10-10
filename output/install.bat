@echo off

rem called by zip installer
if exist "weasel\%IME_FILE%" cd weasel

set IME_FILE=weasels.ime
if /i "%1" == "/t" set IME_FILE=weaselt.ime

echo stopping service.
call stop_service.bat

echo registering Weasel ime.

check_windows_version.js
if errorlevel 1 goto xp_install

:win7_install
elevate rundll32 "%CD%\%IME_FILE%" install
if %ERRORLEVEL% EQU 0 goto success

:xp_install
rundll32 "%CD%\%IME_FILE%" install
if %ERRORLEVEL% EQU 0 goto success

:success

rem start notepad README.txt

:exit
