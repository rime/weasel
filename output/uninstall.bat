@echo off
call stop_service.bat

echo uninstalling Weasel ime.

check_windows_version.js
if errorlevel 1 goto xp_install

:win7_uninstall
elevate rundll32 "%CD%\weasels.ime" uninstall
goto exit

:xp_uninstall
rundll32 "%CD%\weasels.ime" uninstall
goto exit

:exit
