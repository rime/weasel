set work=%cd%
cd %work%\..\librime
call vcbuild.bat
cd %work%
output\weaselserver /q
devenv weasel.sln /Build "ReleaseHant|x64"
if errorlevel 1 goto error
devenv weasel.sln /Build "ReleaseHant|Win32"
if errorlevel 1 goto error
devenv weasel.sln /Build "Release|x64"
if errorlevel 1 goto error
devenv weasel.sln /Build "Release|Win32"
if errorlevel 1 goto error
goto end
:error
echo error building weasel...
:end
cd %work%
