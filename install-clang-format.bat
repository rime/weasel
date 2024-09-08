@echo off

if "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
    rem 64 bit system
    rem powershell -Command "(New-Object Net.WebClient).DownloadFile('https://github.com/llvm/llvm-project/releases/download/llvmorg-18.1.6/LLVM-18.1.6-win64.exe', 'LLVM-18.1.6-win64.exe')"
    rem LLVM-18.1.6-win64.exe /S
    rem or maybe
    rem output/7z.exe e LLVM-18.1.6-win64.exe bin/clang-format.exe -o.
    rem rm LLVM-18.1.6-win64.exe
) else (
    rem 32 bit system
    rem powershell -Command "(New-Object Net.WebClient).DownloadFile('https://github.com/llvm/llvm-project/releases/download/llvmorg-18.1.6/LLVM-18.1.6-win32.exe', 'LLVM-18.1.6-win32.exe')"
    rem LLVM-18.1.6-win32.exe /S
    rem or maybe
    rem output/7z.exe e LLVM-18.1.6-win32.exe bin/clang-format.exe -o.
    rem rm LLVM-18.1.6-win32.exe
)
pause
