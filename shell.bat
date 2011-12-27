@echo off
if exist env.bat call env.bat
set OLD_PATH=%PATH%
if defined DEV_PATH set PATH=%OLD_PATH%;%DEV_PATH%
rem %comspec%
rem %comspec% /k ""C:\Program Files\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"" x86
%comspec% /k ""C:\Program Files\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"" x86
