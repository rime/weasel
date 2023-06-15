@echo off

setlocal

if not exist env.bat copy env.bat.template env.bat

if exist env.bat call env.bat

if not defined WEASEL_VERSION set WEASEL_VERSION=0.15.0
if not defined WEASEL_BUILD set WEASEL_BUILD=0
if not defined WEASEL_ROOT set WEASEL_ROOT=%CD%

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

if not defined BJAM_TOOLSET (
  rem the number actually means platform toolset, not %VisualStudioVersion%
  set BJAM_TOOLSET=msvc-14.2
)

if not defined PLATFORM_TOOLSET (
  set PLATFORM_TOOLSET=v142
)

if defined DEVTOOLS_PATH set PATH=%DEVTOOLS_PATH%%PATH%

set build_config=Release
set build_option=/t:Build
set build_boost=0
set boost_build_variant=release
set build_data=0
set build_opencc=0
set build_hant=0
set build_rime=0
set rime_build_variant=release
set build_weasel=0
set build_installer=0

:parse_cmdline_options
if "%1" == "" goto end_parsing_cmdline_options
if "%1" == "debug" (
  set build_config=Debug
  set boost_build_variant=debug
  set rime_build_variant=debug
)
if "%1" == "release" (
  set build_config=Release
  set boost_build_variant=release
  set rime_build_variant=release
)
if "%1" == "rebuild" set build_option=/t:Rebuild
if "%1" == "boost" set build_boost=1
if "%1" == "data" set build_data=1
if "%1" == "opencc" set build_opencc=1
if "%1" == "hant" set build_hant=1
if "%1" == "rime" set build_rime=1
if "%1" == "librime" set build_rime=1
if "%1" == "weasel" set build_weasel=1
if "%1" == "installer" set build_installer=1
if "%1" == "all" (
  set build_boost=1
  set build_data=1
  set build_opencc=1
  set build_hant=1
  set build_rime=1
  set build_weasel=1
  set build_installer=1
)
shift
goto parse_cmdline_options
:end_parsing_cmdline_options

if %build_weasel% == 0 (
if %build_boost% == 0 (
if %build_data% == 0 (
if %build_opencc% == 0 (
if %build_rime% == 0 (
  set build_weasel=1
)))))

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
  if not exist librime\build.bat (
    git submodule update --init --recursive
  )

  cd %WEASEL_ROOT%\librime
  if not exist env.bat (
    copy %WEASEL_ROOT%\env.bat env.bat
  )
  if not exist lib\opencc.lib (
    call build.bat deps %rime_build_variant%
    if errorlevel 1 goto error
  )
  call build.bat %rime_build_variant%
  if errorlevel 1 goto error

cd %WEASEL_ROOT%
  copy /Y librime\dist\include\rime_*.h include\
  if errorlevel 1 goto error
  copy /Y librime\dist\lib\rime.lib lib\
  if errorlevel 1 goto error
  copy /Y librime\dist\lib\rime.dll output\
  if errorlevel 1 goto error
)

if %build_weasel% == 1 (
  if not exist output\data\essay.txt (
    set build_data=1
  )
  if not exist output\data\opencc\TSCharacters.ocd* (
    set build_opencc=1
  )
)
if %build_data% == 1 call :build_data
if %build_opencc% == 1 call :build_opencc_data

if %build_weasel% == 0 goto end

cd /d %WEASEL_ROOT%

set WEASEL_PROJECT_PROPERTIES=BOOST_ROOT PLATFORM_TOOLSET

if not exist weasel.props (
  cscript.exe render.js weasel.props %WEASEL_PROJECT_PROPERTIES%
)

del msbuild*.log

if %build_hant% == 1 (
  msbuild.exe weasel.sln %build_option% /p:Configuration=ReleaseHant /p:Platform="x64" /fl4
  if errorlevel 1 goto error
  msbuild.exe weasel.sln %build_option% /p:Configuration=ReleaseHant /p:Platform="Win32" /fl3
  if errorlevel 1 goto error
)

msbuild.exe weasel.sln %build_option% /p:Configuration=%build_config% /p:Platform="x64" /fl2
if errorlevel 1 goto error
msbuild.exe weasel.sln %build_option% /p:Configuration=%build_config% /p:Platform="Win32" /fl1
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

set BJAM_OPTIONS_COMMON=-j%NUMBER_OF_PROCESSORS%^
 --with-filesystem^
 --with-json^
 --with-locale^
 --with-regex^
 --with-serialization^
 --with-system^
 --with-thread^
 define=BOOST_USE_WINAPI_VERSION=0x0603^
 toolset=%BJAM_TOOLSET%^
 link=static^
 runtime-link=static^
 --build-type=complete

set BJAM_OPTIONS_X86=%BJAM_OPTIONS_COMMON%^
 architecture=x86^
 address-model=32

set BJAM_OPTIONS_X64=%BJAM_OPTIONS_COMMON%^
 architecture=x86^
 address-model=64

cd /d %BOOST_ROOT%
if not exist b2.exe call bootstrap.bat
if errorlevel 1 goto error
b2 %BJAM_OPTIONS_X86% stage %BOOST_COMPILED_LIBS%
if errorlevel 1 goto error
b2 %BJAM_OPTIONS_X64% stage %BOOST_COMPILED_LIBS%
if errorlevel 1 goto error
exit /b

:build_data
copy %WEASEL_ROOT%\LICENSE.txt output\
copy %WEASEL_ROOT%\README.md output\README.txt
copy %WEASEL_ROOT%\plum\rime-install.bat output\
set plum_dir=plum
set rime_dir=output/data
set WSLENV=plum_dir:rime_dir
bash plum/rime-install %WEASEL_BUNDLED_RECIPES%
if errorlevel 1 goto error
exit /b

:build_opencc_data
if not exist %WEASEL_ROOT%\librime\share\opencc\TSCharacters.ocd2 (
  cd %WEASEL_ROOT%\librime
  call build.bat deps %rime_build_variant%
  if errorlevel 1 goto error
)
cd %WEASEL_ROOT%
if not exist output\data\opencc mkdir output\data\opencc
copy %WEASEL_ROOT%\librime\share\opencc\*.* output\data\opencc\
if errorlevel 1 goto error
exit /b

:error
echo error building weasel...

:end
cd %WEASEL_ROOT%
