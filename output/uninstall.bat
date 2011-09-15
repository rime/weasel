@echo off
call env.bat
call stop_service.bat

echo uninstalling Weasel ime.

ver | python check_osver.py
if %ERRORLEVEL% EQU 5 goto xp_uninstall

:win7_uninstall
elevate rundll32 "%CD%\weasels.ime" uninstall
goto exit

:xp_uninstall
rundll32 "%CD%\weasels.ime" uninstall
goto exit

:exit
