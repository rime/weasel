@echo off

set IME_FILE=weasels.ime
if /i "%1" == "/t" set IME_FILE=weaselt.ime

rem called by zip installer
if exist "weasel\%IME_FILE%" cd weasel

check_python
call env.bat

python --version 2> nul
if %ERRORLEVEL% EQU 0 goto python_ok

if exist python-2.7.msi goto install_python

echo error: cannot locate Python installer.
pause
goto exit

:install_python
echo running Python installer.
python-2.7.msi

check_python
call env.bat

:python_ok

if exist "%UserProfile%\.ibus\zime\zime.db" goto db_ok

echo stopping service.
call stop_service.bat

echo creating database.
call install_schema.bat

:db_ok

echo registering Weasel ime.

ver | python check_osver.py
if %ERRORLEVEL% EQU 5 goto xp_install

:win7_install
elevate rundll32 "%CD%\%IME_FILE%" install
goto success

:xp_install
rundll32 "%CD%\%IME_FILE%" install
goto success

:success

start notepad README.txt

:exit
