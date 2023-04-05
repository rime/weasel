@echo off

rem argument 1: [ /s | /t ] register ime as zh_CN | zh_TW keyboard layout
set install_option=/s
if /i "%1" == "/t" set install_option=/t

set CD_BACK=%CD%
cd "%~dp0"

if /i "%2" == "/register" goto register

echo stopping service from an older version.
call stop_service.bat

echo configuring preset input schemas...
WeaselDeployer.exe /install

echo administrative permissions required. detecting permissions...
net session >nul 2>&1
if not %errorlevel% == 0 (
  echo elevating command prompt...
  cscript sudo.js "%~nx0" %install_option% /register
  exit /b
)

:register
echo registering Weasel IME to your system.
echo install_option=%install_option%

cscript check_windows_version.js
if errorlevel 2 goto win7_x64_install
if errorlevel 1 goto xp_install

:win7_install
WeaselSetup.exe %install_option%
rem regsvr32.exe /s "%CD%\weasel.dll"
goto next

:win7_x64_install
WeaselSetup.exe %install_option%
rem regsvr32.exe /s "%CD%\weasel.dll"
rem regsvr32.exe /s "%CD%\weaselx64.dll"
goto next

:xp_install
WeaselSetup.exe %install_option%
goto next

:next
reg add "HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Run" /v WeaselServer /t REG_SZ /d "%CD%\WeaselServer.exe" /f

:done
start WeaselServer.exe

if /i "%2" == "/register" pause
echo installed.
cd "%CD_BACK%"
