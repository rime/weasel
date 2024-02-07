setlocal

git submodule init
git submodule update plum

rem set rime_version=1.9.0
rem 
rem set download_archive=rime-a608767-Windows-msvc.7z
rem set download_archive_deps=rime-deps-a608767-Windows-msvc.7z

setlocal enabledelayedexpansion
rem fetch librime nightly build 
set rime_version=latest
pushd librime
git pull -v origin master
popd
set "filePath=.git/modules/librime/refs/heads/master"

for /f "delims=" %%i in (%filePath%) do (
    set "fileContent=%%i"
    set "latestCommitId=!fileContent:~0,7!"
    goto :breakLoop
)
:breakLoop
set download_archive=rime-%latestCommitId%-Windows-msvc.7z
set download_archive_deps=rime-deps-%latestCommitId%-Windows-msvc.7z

curl -LO https://github.com/rime/librime/releases/download/%rime_version%/%download_archive%
curl -LO https://github.com/rime/librime/releases/download/%rime_version%/%download_archive_deps%

7z x %download_archive% * -olibrime\ -y
7z x %download_archive_deps% * -olibrime\ -y

copy /Y librime\dist\include\rime_*.h include\
copy /Y librime\dist\lib\rime.lib lib\
copy /Y librime\dist\lib\rime.dll output\

if not exist output\data\opencc mkdir output\data\opencc
copy /Y librime\share\opencc\*.* output\data\opencc\

