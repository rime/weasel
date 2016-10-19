setlocal

"C:\Program Files (x86)\NSIS\Unicode\makensis.exe" output\install.nsi
if %ERRORLEVEL% NEQ 0 goto ERROR
"C:\Program Files (x86)\NSIS\Unicode\makensis.exe" output\expansion.nsi
if %ERRORLEVEL% NEQ 0 goto ERROR

:ERROR
set EXITCODE=%ERRORLEVEL%

:EXIT
exit /b %EXITCODE%
