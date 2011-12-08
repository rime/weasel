set work=%cd%
cd %work%\..\librime
call vcbuild.bat
if errorlevel 1 goto error
cd %work%
output\weaselserver /q
del Release\RimeWithWeasel.lib
devenv weasel.sln /Build "Release|Win32"
rem devenv weasel.sln /Build "ReleaseHant|Win32"
rem devenv weasel.sln /Build "Release|x64"
rem devenv weasel.sln /Build "ReleaseHant|x64"
if errorlevel 0 goto ok
:error
echo error building weasel...
:ok
cd %work%
