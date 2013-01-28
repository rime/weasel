set work=%cd%

cd %work%\librime
call vcbuild.bat

cd %work%
if exist output\weaselserver.exe (
  output\weaselserver.exe /q
)

set build_option=/Build
set build_all=1

:parse_cmdline_options
if "%1" == "" goto end_parsing_cmdline_options
if "%1" == "rebuild" set build_option=/Rebuild
if "%1" == "all" set build_all=1
if "%1" == "some" set build_all=0
shift
goto parse_cmdline_options
:end_parsing_cmdline_options

if %build_all% == 1 (
  copy %work%\LICENSE.txt output\
  copy %work%\README.txt output\
  copy %work%\brise\essay.kct output\data\
  copy %work%\brise\default.yaml output\data\
  copy %work%\brise\symbols.yaml output\data\
  copy %work%\brise\preset\*.yaml output\data\
  copy %work%\brise\supplement\*.yaml output\data\
  copy %work%\brise\extra\*.yaml output\expansion\
)

if %build_all% == 1 (
  devenv weasel.sln %build_option% "ReleaseHant|x64"
  if errorlevel 1 goto error
  devenv weasel.sln %build_option% "ReleaseHant|Win32"
  if errorlevel 1 goto error
)

devenv weasel.sln %build_option% "Release|x64"
if errorlevel 1 goto error
devenv weasel.sln %build_option% "Release|Win32"
if errorlevel 1 goto error
goto end

:error
echo error building weasel...

:end
cd %work%
