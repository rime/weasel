@echo off

set CD_BACK=%CD%
cd "%~dp0"
call stop_service.bat

echo uninstalling Weasel ime.

wscript check_windows_version.js
if errorlevel 2 goto win7_x64_uninstall
if errorlevel 1 goto xp_uninstall

:win7_uninstall
wscript sudo.js WeaselSetup.exe /u
wscript sudo.js regsvr32.exe /s /u "%CD%\weasel.dll"
goto exit

:win7_x64_uninstall
wscript sudo.js WeaselSetupx64.exe /u
wscript sudo.js regsvr32.exe /s /u "%CD%\weasel.dll"
wscript sudo.js regsvr32.exe /s /u "%CD%\weaselx64.dll"
goto exit

:xp_uninstall
WeaselSetup.exe /u
goto exit

:exit
cd "%CD_BACK%"
