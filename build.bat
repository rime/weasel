@echo off

setlocal

set work=%cd%

set build_option=/t:Build
set build_boost=0
set build_data=0
set build_hant=0
set build_rime=0
set build_x64=1

:parse_cmdline_options
if "%1" == "" goto end_parsing_cmdline_options
if "%1" == "rebuild" set build_option=/t:Rebuild
if "%1" == "boost" set build_boost=1
if "%1" == "data" set build_data=1
if "%1" == "hant" set build_hant=1
if "%1" == "rime" set build_rime=1
if "%1" == "librime" set build_rime=1
if "%1" == "all" (
  set build_boost=1
  set build_data=1
  set build_hant=1
  set build_rime=1
)
if "%1" == "nox64" set build_x64=0
shift
goto parse_cmdline_options
:end_parsing_cmdline_options

cd %work%
if exist output\weaselserver.exe (
  output\weaselserver.exe /q
)

if %build_boost% == 1 (
  call :build_boost
  if errorlevel 1 exit /b 1
  cd %work%
)

if %build_rime% == 1 (
  cd %work%\librime
  if not exist librime\thirdparty\lib\opencc.lib (
    call build.bat thirdparty
  )
  call build.bat
  cd %work%
  rem copy /Y librime\thirdparty\lib\*.lib lib\
  copy /Y librime\build\include\rime_*.h include\
  copy /Y librime\build\lib\Release\rime.dll output\
)

if %build_data% == 1 (
  call :build_data
) else if not exist output\data\essay.txt (
  call :build_data
)

del msbuild*.log

if %build_hant% == 1 (
  if %build_x64% == 1 (
    msbuild.exe weasel.sln %build_option% /p:Configuration=ReleaseHant /p:Platform="x64" /fl4
    if errorlevel 1 goto error
  )
  msbuild.exe weasel.sln %build_option% /p:Configuration=ReleaseHant /p:Platform="Win32" /fl3
  if errorlevel 1 goto error
)

if %build_x64% == 1 (
  msbuild.exe weasel.sln %build_option% /p:Configuration=Release /p:Platform="x64" /fl2
  if errorlevel 1 goto error
)
msbuild.exe weasel.sln %build_option% /p:Configuration=Release /p:Platform="Win32" /fl1
if errorlevel 1 goto error
goto end

:build_boost

if not defined BOOST_ROOT (
  echo Error: Boost not found! Please set BOOST_ROOT in command line.
  exit /b 1
) else if not exist "%BOOST_ROOT%\boost" (
  echo Error: Boost headers not found! Please ensure BOOST_ROOT points to libboost root directory.
  exit /b 1
)

set BJAM_TOOLSET=msvc-%VisualStudioVersion%
if %BJAM_TOOLSET% == msvc- (
  echo Error: Please run Visual Studio Developer Command Prompt first.
  exit /b 1
) else if %BJAM_TOOLSET% == msvc-15.0 (
  rem Visual Studio 2017
  set BJAM_TOOLSET=msvc-14.1
)

set boost_build_flags_common=toolset=%BJAM_TOOLSET%^
 variant=release^
 link=static^
 threading=multi^
 runtime-link=static^
 cxxflags="/Zc:threadSafeInit- "

set boost_build_flags_x86=%boost_build_flags_common%^
 define=BOOST_USE_WINAPI_VERSION=0x0501

set boost_build_flags_x64=%boost_build_flags_common%^
 define=BOOST_USE_WINAPI_VERSION=0x0502^
 address-model=64^
 --stagedir=stage_x64

set boost_libs=--with-date_time^
 --with-filesystem^
 --with-locale^
 --with-regex^
 --with-signals^
 --with-system^
 --with-thread

cd %BOOST_ROOT%
if not exist bjam.exe call bootstrap.bat
if %ERRORLEVEL% NEQ 0 goto error
bjam %boost_build_flags_x86% stage %boost_libs%
if %ERRORLEVEL% NEQ 0 goto error
if %build_x64% == 1 (
  bjam %boost_build_flags_x64% stage %boost_libs%
  if %ERRORLEVEL% NEQ 0 goto error
)
exit /b

:build_data
rem call :build_essay
copy %work%\LICENSE.txt output\
copy %work%\README.md output\README.txt
copy %work%\brise\essay.txt output\data\
copy %work%\brise\default.yaml output\data\
copy %work%\brise\symbols.yaml output\data\
copy %work%\brise\preset\*.yaml output\data\
copy %work%\brise\supplement\*.yaml output\data\
if not exist output\expansion mkdir output\expansion
copy %work%\brise\extra\*.yaml output\expansion\
call :build_opencc_data
exit /b

:build_essay
rem essay.kct is deprecated.
copy %work%\librime\thirdparty\bin\kctreemgr.exe %work%\brise\
copy %work%\librime\thirdparty\bin\zlib1.dll %work%\brise\
cd %work%\brise
call make_essay.bat
cd %work%
exit /b

:build_opencc_data
if not exist %work%\librime\thirdparty\data\opencc\TSCharacters.ocd (
  cd %work%\librime
  call build.bat thirdparty
)
cd %work%
if not exist output\data\opencc mkdir output\data\opencc
copy %work%\librime\thirdparty\data\opencc\*.* output\data\opencc\
exit /b

:error
echo error building weasel...

:end
cd %work%
