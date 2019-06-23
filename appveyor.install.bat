setlocal

call appveyor_build_boost.bat
if errorlevel 1 goto error

git submodule init
git submodule update plum

set librime_version=1.5.3
set download_archive=rime-with-plugins-%librime_version%-win32.zip
appveyor DownloadFile https://github.com/rime/librime/releases/download/%librime_version%/%download_archive%
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
