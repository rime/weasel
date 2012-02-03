set work=%cd%
cd %work%\..\librime
call vcbuild.bat
cd %work%
output\weaselserver /q
copy %work%\..\brise\essay.kct output\data\
copy %work%\..\brise\default.yaml output\data\
copy %work%\..\brise\preset\*.yaml output\data\
rem devenv weasel.sln /Build "ReleaseHant|x64"
rem if errorlevel 1 goto error
rem devenv weasel.sln /Build "ReleaseHant|Win32"
rem if errorlevel 1 goto error
rem devenv weasel.sln /Build "Release|x64"
rem if errorlevel 1 goto error
devenv weasel.sln /Build "Release|Win32"
if errorlevel 1 goto error
goto end
:error
echo error building weasel...
:end
cd %work%
