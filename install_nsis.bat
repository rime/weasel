set nsis_version=3.08

mkdir deps
powershell -c "iwr -UserAgent \"Wget\" -Uri \"https://sourceforge.net/projects/nsis/files/NSIS%%203/%nsis_version%/nsis-%nsis_version%-setup.exe/download\" -OutFile deps\nsis-setup.exe"
deps\nsis-setup.exe /S

