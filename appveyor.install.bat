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

git submodule init
git submodule update plum
rem librime 1.5.0
appveyor DownloadFile https://github.com/rime/librime/releases/download/1.5.0/rime-with-plugins.zip
7z x rime-with-plugins.zip * -olibrime\ | find "ing archive"
copy /Y librime\dist\include\rime_*.h include\
copy /Y librime\dist\lib\rime.lib lib\
copy /Y librime\dist\lib\rime.dll output\
if not exist output\data\opencc mkdir output\data\opencc
copy /Y librime\thirdparty\share\opencc\*.* output\data\opencc\

if %nocache% == 1 (
	pushd %BOOST_ROOT%
	call .\bootstrap.bat
	.\b2.exe %boost_build_options_x86% %boost_compiled_libs% -q -d0 stage
	.\b2.exe %boost_build_options_x64% %boost_compiled_libs% -q -d0 stage
	popd
	if %ERRORLEVEL% NEQ 0 goto ERROR

	date /t > boost.cached & time /t >> boost.cached
	echo.
	echo Boost libraries installed.
	echo.
	goto EXIT
) else (
	echo.
	echo Last build date of cache is
	type boost.cached
	echo.
	goto EXIT
)

:ERROR
set EXITCODE=%ERRORLEVEL%

:EXIT
exit /b %EXITCODE%
