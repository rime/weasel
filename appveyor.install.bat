setlocal

set nocache=0

if not exist boost.cached set nocache=1
if not exist %BOOST_ROOT% set nocache=1

git submodule init
git submodule update brise
rem librime 1.2.10
appveyor DownloadFile https://ci.appveyor.com/api/buildjobs/a952q4qoye3ebufj/artifacts/rime.zip
7z x rime.zip * -olibrime\ | find "ing archive"
copy /Y librime\build\include\rime_*.h include\
copy /Y librime\build\lib\Release\rime.dll output\

if %nocache% == 1 (
	pushd C:\Libraries
	appveyor DownloadFile http://superb-sea2.dl.sourceforge.net/project/boost/boost/1.61.0/boost_1_61_0.7z
	7z x boost_1_61_0.7z | find "ing archive"
	cd boost_1_61_0
	call .\bootstrap.bat
	call .\b2.exe --prefix=%BOOST_ROOT% toolset=msvc-14.0 variant=release link=static threading=multi runtime-link=static define=BOOST_USE_WINAPI_VERSION=0x0501 cxxflags="/Zc:threadSafeInit- " --with-date_time --with-filesystem --with-locale --with-regex --with-signals --with-system --with-thread -q -d0 install
	xcopy /e /i /y /q %BOOST_ROOT%\include\boost-1_61\boost %BOOST_ROOT%\boost
	xcopy /e /i /y /q %BOOST_ROOT%\lib %BOOST_ROOT%\stage\lib
	call .\b2.exe --prefix=%BOOST_ROOT%_x64 toolset=msvc-14.0 variant=release link=static threading=multi runtime-link=static address-model=64 define=BOOST_USE_WINAPI_VERSION=0x0501 cxxflags="/Zc:threadSafeInit- " --with-date_time --with-filesystem --with-locale --with-regex --with-signals --with-system --with-thread -q -d0 install
	xcopy /e /i /y /q %BOOST_ROOT%_x64\lib %BOOST_ROOT%\stage_x64\lib
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
