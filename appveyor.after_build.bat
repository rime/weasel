setlocal

"C:\Program Files (x86)\NSIS\makensis.exe" output\install.nsi
if %ERRORLEVEL% NEQ 0 goto ERROR

:ERROR
set EXITCODE=%ERRORLEVEL%

:EXIT
exit /b %EXITCODE%
