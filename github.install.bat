setlocal

git submodule init
git submodule update plum

set rime_version=1.11.0

set download_archive=rime-76a0a16-Windows-msvc-x86.7z
set download_archive_deps=rime-deps-76a0a16-Windows-msvc-x86.7z
set download_archive_x64=rime-76a0a16-Windows-msvc-x64.7z
set download_archive_deps_x64=rime-deps-76a0a16-Windows-msvc-x64.7z

curl -LO https://github.com/rime/librime/releases/download/%rime_version%/%download_archive%
curl -LO https://github.com/rime/librime/releases/download/%rime_version%/%download_archive_deps%
curl -LO https://github.com/rime/librime/releases/download/%rime_version%/%download_archive_x64%
curl -LO https://github.com/rime/librime/releases/download/%rime_version%/%download_archive_deps_x64%

7z x %download_archive% * -olibrime\ -y
7z x %download_archive_deps% * -olibrime\ -y
7z x %download_archive_x64% * -olibrime_x64\ -y
7z x %download_archive_deps_x64% * -olibrime_x64\ -y

copy /Y librime\dist\include\rime_*.h include\
copy /Y librime\dist\lib\rime.lib lib\
copy /Y librime\dist\lib\rime.dll output\Win32\

copy /Y librime_x64\dist\lib\rime.lib lib64\
copy /Y librime_x64\dist\lib\rime.dll output\

if not exist output\data\opencc mkdir output\data\opencc
copy /Y librime\share\opencc\*.* output\data\opencc\

