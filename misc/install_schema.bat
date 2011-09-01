@echo off
call env.bat
call stop_service.bat

cd data

:menu
echo --------------------------------------------------
echo Pinyin      - Simplified :(
echo TonalPinyin - Traditional :)
echo Zhuyin      - Bopomofo
echo Quick       - A Cangjie variant
echo Jyutping    - Cantonese
echo Wu          - Shanghainese
echo --------------------------------------------------
set Choice=
set /p Choice="what's the schema of your choice [Ptzqjw]? "
if /i "%Choice%" == "" set Choice=pinyin
if /i "%Choice%" == "p" set Choice=pinyin
if /i "%Choice%" == "t" set Choice=tonal_pinyin
if /i "%Choice%" == "z" set Choice=zhuyin
if /i "%Choice%" == "q" set Choice=quick
if /i "%Choice%" == "j" set Choice=jyutping
if /i "%Choice%" == "w" set Choice=wu
set SchemaInstaller="schema_%Choice%.bat"
if exist %SchemaInstaller% (
call %SchemaInstaller%
) else (
echo sorry, "%Choice%" is not available.
goto menu
)

:exit
cd ..
