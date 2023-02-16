setlocal

git submodule init
git submodule update plum

set rime_version=1.8.5

set download_archive=rime-08dd95f-Windows.7z
set download_archive_deps=rime-deps-08dd95f-Windows.7z

curl -LO https://github.com/rime/librime/releases/download/%rime_version%/%download_archive%
curl -LO https://github.com/rime/librime/releases/download/%rime_version%/%download_archive_deps%

7z x %download_archive% * -olibrime\ | find "ing archive"
7z x %download_archive_deps% * -olibrime\ | find "ing archive"

copy /Y librime\dist\include\rime_*.h include\
copy /Y librime\dist\lib\rime.lib lib\
copy /Y librime\dist\lib\rime.dll output\

if not exist output\data\opencc mkdir output\data\opencc
copy /Y librime\share\opencc\*.* output\data\opencc\

