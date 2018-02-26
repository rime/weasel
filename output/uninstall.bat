@echo off

set CD_BACK=%CD%
cd "%~dp0"

if /i "%1" == "/unregister" goto unregister

echo stopping service.
call stop_service.bat

echo administrative permissions required. detecting permissions...
net session >nul 2>&1
if not %errorlevel% == 0 (
  echo elevating command prompt...
  cscript sudo.js "%~nx0" /unregister
  exit /b
)

:unregister
echo uninstalling Weasel ime.

cscript check_windows_version.js
if errorlevel 2 goto win7_x64_uninstall
if errorlevel 1 goto xp_uninstall

:win7_uninstall
WeaselSetup.exe /u
rem regsvr32.exe /s /u "%CD%\weasel.dll"
goto next

:win7_x64_uninstall
WeaselSetupx64.exe /u
rem regsvr32.exe /s /u "%CD%\weasel.dll"
rem regsvr32.exe /s /u "%CD%\weaselx64.dll"
goto next

:xp_uninstall
WeaselSetup.exe /u
goto next

:next
reg delete "HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Run" /v WeaselServer /f

:done
if /i "%1" == "/unregister" pause
echo uninstalled.
cd "%CD_BACK%"
