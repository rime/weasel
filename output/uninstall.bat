@echo off
call stop_service.bat

echo uninstalling Weasel ime.

wscript check_windows_version.js
if errorlevel 2 goto win7_x64_uninstall
if errorlevel 1 goto xp_uninstall

:win7_uninstall
wscript elevate.js rundll32 "%CD%\weasel.ime" uninstall
goto exit

:win7_x64_uninstall
wscript elevate.js rundll32 "%CD%\weaselx64.ime" uninstall
goto exit

:xp_uninstall
rundll32 "%CD%\weasel.ime" uninstall
goto exit

:exit
