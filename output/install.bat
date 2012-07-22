@echo off

rem argument 1: [ /s | /t ] register ime as zh_CN | zh_TW keyboard layout
set WEASEL_INSTALL_OPTION=/s
if /i "%1" == "/t" set WEASEL_INSTALL_OPTION=/t

cd "%~dp0"

echo stopping service from an older version.
call stop_service.bat

echo configuring preset input schemas...
WeaselDeployer.exe /install

echo registering Weasel IME to your system.

wscript check_windows_version.js
if errorlevel 2 goto win7_x64_install
if errorlevel 1 goto xp_install

:win7_install
wscript elevate.js WeaselSetup.exe %WEASEL_INSTALL_OPTION%
if %ERRORLEVEL% EQU 0 goto success

:win7_x64_install
wscript elevate.js WeaselSetupx64.exe %WEASEL_INSTALL_OPTION%
if %ERRORLEVEL% EQU 0 goto success

:xp_install
WeaselSetup.exe %WEASEL_INSTALL_OPTION%
if %ERRORLEVEL% EQU 0 goto success

:success

rem start notepad README.txt

:exit
