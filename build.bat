@echo off

setlocal

set WEASEL_VERSION=0.14.3
if not defined WEASEL_BUILD set WEASEL_BUILD=0
if not defined WEASEL_ROOT set WEASEL_ROOT=%CD%

if exist env.bat call env.bat

echo WEASEL_VERSION=%WEASEL_VERSION%
echo WEASEL_BUILD=%WEASEL_BUILD%
echo WEASEL_ROOT=%WEASEL_ROOT%
echo WEASEL_BUNDLED_RECIPES=%WEASEL_BUNDLED_RECIPES%
echo.

if defined BOOST_ROOT (
  if exist "%BOOST_ROOT%\boost" goto boost_found
)
echo Error: Boost not found! Please set BOOST_ROOT in env.bat.
exit /b 1
:boost_found
echo BOOST_ROOT=%BOOST_ROOT%
echo.

if defined BASH_ROOT (
  if exist "%BASH_ROOT%\bash.exe" goto bash_found
)
echo Error: Bash not found! Please set BASH_ROOT in env.bat.
exit /b 1
:bash_found
echo BASH_ROOT=%BASH_ROOT%
echo.

if not defined BJAM_TOOLSET (
  rem the number actually means platform toolset, not %VisualStudioVersion%
  set BJAM_TOOLSET=msvc-14.1
)

if not defined PLATFORM_TOOLSET (
  set PLATFORM_TOOLSET=v141_xp
)

if not defined MSBUILD_PATH (
  for /f "usebackq tokens=*" %%i in (`vswhere -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
    set MSBUILD_PATH="%%i"
    REM exit /b !errorlevel!
  )
)
echo.
echo MSBUILD_PATH=%MSBUILD_PATH%
echo.

if defined DEVTOOLS_PATH set PATH=%DEVTOOLS_PATH%%PATH%

set build_config=Release
set build_option=/t:Build
set build_boost=0
set build_boost_variant=release
set build_data=0
set build_opencc=0
set build_hant=0
set build_rime=0
set build_installer=0
set build_x64=1

:parse_cmdline_options
if "%1" == "" goto end_parsing_cmdline_options
if "%1" == "debug" (
  set build_config=Debug
  set build_boost_variant=debug
)
if "%1" == "release" (
  set build_config=Release
  set build_boost_variant=release
)
if "%1" == "rebuild" set build_option=/t:Rebuild
if "%1" == "boost" set build_boost=1
if "%1" == "data" set build_data=1
if "%1" == "opencc" set build_opencc=1
if "%1" == "hant" set build_hant=1
if "%1" == "rime" set build_rime=1
if "%1" == "librime" set build_rime=1
if "%1" == "installer" set build_installer=1
if "%1" == "all" (
  set build_boost=1
  set build_data=1
  set build_opencc=1
  set build_hant=1
  set build_rime=1
  set build_installer=1
)
if "%1" == "nox64" set build_x64=0
shift
goto parse_cmdline_options
:end_parsing_cmdline_options

cd /d %WEASEL_ROOT%
if exist output\weaselserver.exe (
  output\weaselserver.exe /q
)

if %build_boost% == 1 (
  call :build_boost
  if errorlevel 1 exit /b 1
  cd /d %WEASEL_ROOT%
)

if %build_rime% == 1 (
  cd %WEASEL_ROOT%\librime
  if not exist librime\thirdparty\lib\opencc.lib (
    call build.bat thirdparty
    if errorlevel 1 goto error
  )
  call build.bat
  if errorlevel 1 goto error
  cd %WEASEL_ROOT%
  copy /Y librime\dist\include\rime_*.h include\
  if errorlevel 1 goto error
  copy /Y librime\dist\lib\rime.lib lib\
  if errorlevel 1 goto error
  copy /Y librime\dist\lib\rime.dll output\
  if errorlevel 1 goto error
)


if not exist output\data\essay.txt set build_data=1
if %build_data% == 1 call :build_data

if not exist output\data\opencc\TSCharacters.ocd set build_opencc=1
if %build_opencc% == 1 call :build_opencc_data

cd /d %WEASEL_ROOT%

set WEASEL_PROJECT_PROPERTIES=BOOST_ROOT PLATFORM_TOOLSET

if not exist weasel.props (
  cscript.exe render.js weasel.props %WEASEL_PROJECT_PROPERTIES%
)

del msbuild*.log

if %build_hant% == 1 (
  if %build_x64% == 1 (
    %MSBUILD_PATH% weasel.sln %build_option% /p:Configuration=ReleaseHant /p:Platform="x64" /fl4
    if errorlevel 1 goto error
  )
  %MSBUILD_PATH% weasel.sln %build_option% /p:Configuration=ReleaseHant /p:Platform="Win32" /fl3
  if errorlevel 1 goto error
)

if %build_x64% == 1 (
  %MSBUILD_PATH% weasel.sln %build_option% /p:Configuration=%build_config% /p:Platform="x64" /fl2
  if errorlevel 1 goto error
)
%MSBUILD_PATH% weasel.sln %build_option% /p:Configuration=%build_config% /p:Platform="Win32" /fl1
if errorlevel 1 goto error

if %build_installer% == 1 (
  "%ProgramFiles(x86)%"\NSIS\Bin\makensis.exe ^
  /DWEASEL_VERSION=%WEASEL_VERSION% ^
  /DWEASEL_BUILD=%WEASEL_BUILD% ^
  output\install.nsi
  if errorlevel 1 goto error
)

goto end

:build_boost

set BOOST_COMPILED_LIBS=--with-date_time^
 --with-filesystem^
 --with-locale^
 --with-regex^
 --with-system^
 --with-thread^
 --with-serialization

set BJAM_OPTIONS_COMMON=toolset=%BJAM_TOOLSET%^
 variant=%build_boost_variant%^
 link=static^
 threading=multi^
 runtime-link=static^
 cxxflags="/Zc:threadSafeInit- "

set BJAM_OPTIONS_X86=%BJAM_OPTIONS_COMMON%^
 define=BOOST_USE_WINAPI_VERSION=0x0501

set BJAM_OPTIONS_X64=%BJAM_OPTIONS_COMMON%^
 define=BOOST_USE_WINAPI_VERSION=0x0502^
 address-model=64^
 --stagedir=stage_x64

cd /d %BOOST_ROOT%
if not exist bjam.exe call bootstrap.bat
if errorlevel 1 goto error
bjam %BJAM_OPTIONS_X86% stage %BOOST_COMPILED_LIBS%
if errorlevel 1 goto error
if %build_x64% == 1 (
  bjam %BJAM_OPTIONS_X64% stage %BOOST_COMPILED_LIBS%
  if errorlevel 1 goto error
)
exit /b

:build_data
rem call :build_essay
copy %WEASEL_ROOT%\LICENSE.txt output\
copy %WEASEL_ROOT%\README.md output\README.txt
copy %WEASEL_ROOT%\plum\rime-install.bat output\
set plum_dir=plum
set rime_dir=output/data
"%BASH_ROOT%\bash.exe" plum/rime-install %WEASEL_BUNDLED_RECIPES%
if errorlevel 1 goto error
exit /b

:build_essay
rem essay.kct is deprecated.
cd %WEASEL_ROOT%\plum
copy %WEASEL_ROOT%\librime\thirdparty\bin\kctreemgr.exe .\
copy %WEASEL_ROOT%\librime\thirdparty\bin\zlib1.dll .\
call make_essay.bat
cd %WEASEL_ROOT%
exit /b

:build_opencc_data
if not exist %WEASEL_ROOT%\librime\thirdparty\share\opencc\TSCharacters.ocd (
  cd %WEASEL_ROOT%\librime
  call build.bat thirdparty
  if errorlevel 1 goto error
)
cd %WEASEL_ROOT%
if not exist output\data\opencc mkdir output\data\opencc
copy %WEASEL_ROOT%\librime\thirdparty\share\opencc\*.* output\data\opencc\
if errorlevel 1 goto error
exit /b

:error
echo error building weasel...
exit /b

:end
echo Finish building weasel!
cd %WEASEL_ROOT%
