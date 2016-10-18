setlocal

if exist env.bat call env.bat

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
  copy /Y librime\thirdparty\bin\*.dll output\
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
set boost_build_flags=toolset=msvc-14.0 variant=release link=static threading=multi runtime-link=static
set boost_libs=--with-date_time --with-filesystem --with-locale --with-regex --with-signals --with-thread
cd %BOOST_ROOT%
if not exist bjam.exe call bootstrap.bat
if %ERRORLEVEL% NEQ 0 goto error
bjam %boost_build_flags% stage %boost_libs%
if %ERRORLEVEL% NEQ 0 goto error
if %build_x64% == 1 (
  bjam %boost_build_flags% address-model=64 --stagedir=stage_x64 stage %boost_libs%
  if %ERRORLEVEL% NEQ 0 goto error
)
exit /b

:build_data
rem call :build_essay
copy %work%\LICENSE.txt output\
copy %work%\README.txt output\
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

:error
echo error building weasel...

:end
cd %work%
