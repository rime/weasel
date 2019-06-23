setlocal

set boost_compiled_libs=--with-date_time^
 --with-filesystem^
 --with-locale^
 --with-regex^
 --with-serialization^
 --with-system^
 --with-thread

set boost_build_options_common=toolset=msvc-14.1^
 variant=release^
 link=static^
 threading=multi^
 runtime-link=static^
 cxxflags="/Zc:threadSafeInit- "

set boost_build_options_x86=%boost_build_options_common%^
 define=BOOST_USE_WINAPI_VERSION=0x0501

set boost_build_options_x64=%boost_build_options_common%^
 define=BOOST_USE_WINAPI_VERSION=0x0502^
 address-model=64^
 --stagedir=stage_x64

set nocache=0

if not exist boost.cached set nocache=1
if not exist %BOOST_ROOT%/stage set nocache=1
if not exist %BOOST_ROOT%/stage_x64 set nocache=1

if %nocache% == 1 (
	pushd %BOOST_ROOT%
	call .\bootstrap.bat
	.\b2.exe %boost_build_options_x86% %boost_compiled_libs% -q -d0 stage
	.\b2.exe %boost_build_options_x64% %boost_compiled_libs% -q -d0 stage
	popd
	if errorlevel 1 goto error

	date /t > boost.cached & time /t >> boost.cached
	echo.
	echo Boost libraries installed.
	echo.
) else (
	echo.
	echo Last build date of cache is
	type boost.cached
	echo.
)

goto exit

:error
set exitcode=%errorlevel%

:exit
exit /b %exitcode%
