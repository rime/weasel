@echo off

rem argument 1: [ /s | /t ] register ime as zh_CN | zh_TW keyboard layout
set WEASEL_INSTALL_OPTION=/s
if /i "%1" == "/t" set WEASEL_INSTALL_OPTION=/t

set CD_BACK=%CD%
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
wscript sudo.js WeaselSetup.exe %WEASEL_INSTALL_OPTION%
rem wscript sudo.js regsvr32.exe /s "%CD%\weasel.dll"
goto exit

:win7_x64_install
wscript sudo.js WeaselSetupx64.exe %WEASEL_INSTALL_OPTION%
rem wscript sudo.js regsvr32.exe /s "%CD%\weasel.dll"
rem wscript sudo.js regsvr32.exe /s "%CD%\weaselx64.dll"
goto exit

:xp_install
WeaselSetup.exe %WEASEL_INSTALL_OPTION%
goto exit

:exit
cd "%CD_BACK%"
