call env.bat
set OUTPUT=output\weasel
set SERVER=%OUTPUT%\WeaselServer.exe

if exist %SERVER% %SERVER% /q
rmdir /s /q %OUTPUT%
mkdir %OUTPUT%
rem mkdir %OUTPUT%\data
rem copy ..\rime.py\data\*.bat %OUTPUT%\data
rem copy ..\rime.py\data\*.py %OUTPUT%\data
rem copy ..\rime.py\data\*.txt %OUTPUT%\data
rem mkdir %OUTPUT%\engine
rem mkdir %OUTPUT%\engine\ibus
rem copy ..\rime.py\engine\*.py %OUTPUT%\engine
rem copy ..\rime.py\weasel\*.py %OUTPUT%\engine
rem copy ..\rime.py\weasel\ibus\*.py %OUTPUT%\engine\ibus
rem copy misc\*.conf %OUTPUT%\engine
copy misc\*.bat %OUTPUT%
copy misc\*.js %OUTPUT%
copy misc\*.py %OUTPUT%
copy misc\*.reg %OUTPUT%
copy misc\*.txt %OUTPUT%
copy Release\WeaselServer.exe %OUTPUT%
copy Release\weasel.ime %OUTPUT%\weasels.ime
copy ReleaseHant\weasel.ime %OUTPUT%\weaselt.ime
rem for installer
copy misc\zhung.* output
pause
