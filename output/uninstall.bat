@echo off

cd "%~dp0"
call stop_service.bat

echo uninstalling Weasel ime.

wscript check_windows_version.js
if errorlevel 2 goto win7_x64_uninstall
if errorlevel 1 goto xp_uninstall

:win7_uninstall
wscript sudo.js WeaselSetup.exe /u
goto exit

:win7_x64_uninstall
wscript sudo.js WeaselSetupx64.exe /u
goto exit

:xp_uninstall
WeaselSetup.exe /u
goto exit

:exit
