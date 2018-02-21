@echo off

setlocal

if exist env.bat call env.bat

if not defined WEASEL_ROOT set WEASEL_ROOT=%CD%
echo WEASEL_ROOT=%WEASEL_ROOT%
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
  set BJAM_TOOLSET=msvc-14.0
)

if not defined PLATFORM_TOOLSET (
  set PLATFORM_TOOLSET=v140_xp
)

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
  )
  call build.bat
  cd %WEASEL_ROOT%
  rem copy /Y librime\thirdparty\lib\*.lib lib\
  copy /Y librime\build\include\rime_*.h include\
  copy /Y librime\build\lib\Release\rime.dll output\
)

if %build_data% == 1 (
  call :build_data
) else if not exist output\data\essay.txt (
  call :build_data
)

cd /d %WEASEL_ROOT%

set WEASEL_PROJECT_PROPERTIES=BOOST_ROOT PLATFORM_TOOLSET

if not exist weasel.props (
  cscript.exe render.js weasel.props %WEASEL_PROJECT_PROPERTIES%
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

set BOOST_COMPILED_LIBS=--with-date_time^
 --with-filesystem^
 --with-locale^
 --with-regex^
 --with-signals^
 --with-system^
 --with-thread

set BJAM_OPTIONS_COMMON=toolset=%BJAM_TOOLSET%^
 variant=release^
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
if %ERRORLEVEL% NEQ 0 goto error
bjam %BJAM_OPTIONS_X86% stage %BOOST_COMPILED_LIBS%
if %ERRORLEVEL% NEQ 0 goto error
if %build_x64% == 1 (
  bjam %BJAM_OPTIONS_X64% stage %BOOST_COMPILED_LIBS%
  if %ERRORLEVEL% NEQ 0 goto error
)
exit /b

:build_data
rem call :build_essay
copy %WEASEL_ROOT%\LICENSE.txt output\
copy %WEASEL_ROOT%\README.md output\README.txt
copy %WEASEL_ROOT%\brise\essay.txt output\data\
copy %WEASEL_ROOT%\brise\default.yaml output\data\
copy %WEASEL_ROOT%\brise\symbols.yaml output\data\
copy %WEASEL_ROOT%\brise\preset\*.yaml output\data\
copy %WEASEL_ROOT%\brise\supplement\*.yaml output\data\
if not exist output\expansion mkdir output\expansion
copy %WEASEL_ROOT%\brise\extra\*.yaml output\expansion\
call :build_opencc_data
exit /b

:build_essay
rem essay.kct is deprecated.
copy %WEASEL_ROOT%\librime\thirdparty\bin\kctreemgr.exe %WEASEL_ROOT%\brise\
copy %WEASEL_ROOT%\librime\thirdparty\bin\zlib1.dll %WEASEL_ROOT%\brise\
cd %WEASEL_ROOT%\brise
call make_essay.bat
cd %WEASEL_ROOT%
exit /b

:build_opencc_data
if not exist %WEASEL_ROOT%\librime\thirdparty\data\opencc\TSCharacters.ocd (
  cd %WEASEL_ROOT%\librime
  call build.bat thirdparty
)
cd %WEASEL_ROOT%
if not exist output\data\opencc mkdir output\data\opencc
copy %WEASEL_ROOT%\librime\thirdparty\data\opencc\*.* output\data\opencc\
exit /b

:error
echo error building weasel...

:end
cd %WEASEL_ROOT%
