@echo off
if exist env.bat call env.bat
set OLD_PATH=%PATH%
if defined DEV_PATH set PATH=%OLD_PATH%;%DEV_PATH%
%comspec% /k ""%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat"" x86
