setlocal

call appveyor_build_boost.bat
if errorlevel 1 goto error

git submodule init
git submodule update plum

set rime_version=1.5.3
set rime_variant=rime-with-plugins

set download_archive=%rime_variant%-%rime_version%-win32.zip
appveyor DownloadFile https://github.com/rime/librime/releases/download/%rime_version%/%download_archive%
7z x %download_archive% * -olibrime\ | find "ing archive"
copy /Y librime\dist\include\rime_*.h include\
copy /Y librime\dist\lib\rime.lib lib\
copy /Y librime\dist\lib\rime.dll output\
if not exist output\data\opencc mkdir output\data\opencc
copy /Y librime\thirdparty\share\opencc\*.* output\data\opencc\

:error
set exitcode=%errorlevel%

:exit
exit /b %exitcode%
