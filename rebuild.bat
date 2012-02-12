set work=%cd%
cd %work%\..\librime
call vcbuild.bat
cd %work%
output\weaselserver /q
copy %work%\..\brise\essay.kct output\data\
copy %work%\..\brise\default.yaml output\data\
copy %work%\..\brise\preset\*.yaml output\data\
copy %work%\..\brise\supplement\*.yaml output\data\
devenv weasel.sln /Rebuild "ReleaseHant|x64"
if errorlevel 1 goto error
devenv weasel.sln /Rebuild "ReleaseHant|Win32"
if errorlevel 1 goto error
devenv weasel.sln /Rebuild "Release|x64"
if errorlevel 1 goto error
devenv weasel.sln /Rebuild "Release|Win32"
if errorlevel 1 goto error
goto end
:error
echo error building weasel...
:end
cd %work%
