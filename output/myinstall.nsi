; weasel installation script
!include FileFunc.nsh
!include LogicLib.nsh
!include x64.nsh

!define WEASEL_VERSION 0.9.30
!define WEASEL_BUILD ${WEASEL_VERSION}.0

!define WEASEL_ROOT $INSTDIR\weasel-${WEASEL_VERSION}

; The name of the installer
Name "С�Ǻ� ${WEASEL_VERSION}"

; The file to write
OutFile "archives\weasel-${WEASEL_BUILD}-CQLB-installer.exe"

VIProductVersion "${WEASEL_BUILD}"
VIAddVersionKey /LANG=2052 "ProductName" "С�Ǻ�"
VIAddVersionKey /LANG=2052 "Comments" "Powered by RIME | ���������뷨����"
VIAddVersionKey /LANG=2052 "CompanyName" "ʽˡ��"
VIAddVersionKey /LANG=2052 "LegalCopyright" "Copyleft RIME Developers"
VIAddVersionKey /LANG=2052 "FileDescription" "С�Ǻ����뷨"
VIAddVersionKey /LANG=2052 "FileVersion" "${WEASEL_VERSION}"

Icon ..\resource\weasel.ico
SetCompressor /SOLID lzma

; The default installation directory
InstallDir $PROGRAMFILES\Rime

; Registry key to check for directory (so if you install again, it will
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\Rime\Weasel" "InstallDir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

LoadLanguageFile "${NSISDIR}\Contrib\Language Files\SimpChinese.nlf"

;--------------------------------

; Pages

;Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

Function .onInit
  ReadRegStr $R0 HKLM \
  "Software\Microsoft\Windows\CurrentVersion\Uninstall\Weasel" \
  "UninstallString"
  StrCmp $R0 "" done

  IfSilent uninst 0
  MessageBox MB_OKCANCEL|MB_ICONINFORMATION \
  "��װǰ���Ҵ�����ж�ؾɰ汾��С�Ǻ���$\n$\n���¡�ȷ�����Ƴ��ɰ汾�����¡�ȡ�����������ΰ�װ��" \
  IDOK uninst
  Abort

uninst:
  ; Backup data directory from previous installation, user files may exist
  ReadRegStr $R1 HKLM SOFTWARE\Rime\Weasel "WeaselRoot"
  StrCmp $R1 "" call_uninstaller
  IfFileExists $R1\data\*.* 0 call_uninstaller
  CreateDirectory $TEMP\weasel-backup
  CopyFiles $R1\data\*.* $TEMP\weasel-backup

call_uninstaller:
  ExecWait '$R0 /S'
  Sleep 800

done:
FunctionEnd

; The stuff to install
Section "Weasel"

  SectionIn RO

  ; Write the new installation path into the registry
  WriteRegStr HKLM SOFTWARE\Rime\Weasel "InstallDir" "$INSTDIR"

  ; Reset INSTDIR for the new version
  StrCpy $INSTDIR "${WEASEL_ROOT}"

  IfFileExists "$INSTDIR\WeaselServer.exe" 0 +2
  ExecWait '"$INSTDIR\WeaselServer.exe" /quit'

  SetOverwrite try
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  IfFileExists $TEMP\weasel-backup\*.* 0 program_files
  CreateDirectory $INSTDIR\data
  CopyFiles $TEMP\weasel-backup\*.* $INSTDIR\data
  RMDir /r $TEMP\weasel-backup

program_files:
  File "LICENSE.txt"
  File "README.txt"
  File "weasel.dll"
  File "weaselx64.dll"
  File "weaselt.dll"
  File "weaseltx64.dll"
  File "weasel.ime"
  File "weaselx64.ime"
  File "weaselt.ime"
  File "weaseltx64.ime"
  File "WeaselDeployer.exe"
  File "WeaselServer.exe"
  File "WeaselSetup.exe"
  File "WeaselSetupx64.exe"
  File "libglog.dll"
  File "opencc.dll"
  File "rime.dll"
  File "WinSparkle.dll"
  File "zlib1.dll"
  ; shared data files
  SetOutPath  $INSTDIR\data
  File "data\default.yaml"
  File "data\symbols.yaml"
  File "data\weasel.yaml"
  File "data\essay.txt"
  File "data\*.schema.yaml"
  File "data\*.dict.yaml"
  ; opencc data files
  SetOutPath  $INSTDIR\data\opencc
  File "data\opencc\*.ini"
  File "data\opencc\*.ocd"
  File "data\opencc\*.txt"
  ; images
  SetOutPath  $INSTDIR\data\preview
  File "data\preview\*.png"
  ;������������Դ��
  SetOutPath  $INSTDIR\������������Դ��\librime\include\rime\gear
  File "������������Դ��\librime\include\rime\gear\*.h"
  SetOutPath  $INSTDIR\������������Դ��\librime\src\gear
  File "������������Դ��\librime\src\gear\*.cc"

  SetOutPath $INSTDIR

  ; test /T flag for zh_TW locale
  StrCpy $R2  "/i"
  ${GetParameters} $R0
  ClearErrors
  ${GetOptions} $R0 "/S" $R1
  IfErrors +2 0
  StrCpy $R2 "/s"
  ${GetOptions} $R0 "/T" $R1
  IfErrors +2 0
  StrCpy $R2 "/t"

  ${If} ${RunningX64}
    ExecWait '"$INSTDIR\WeaselSetupx64.exe" $R2'
  ${Else}
    ExecWait '"$INSTDIR\WeaselSetup.exe" $R2'
  ${EndIf}

  ; run as user...
  ExecWait "$INSTDIR\WeaselDeployer.exe /install"

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Weasel" "DisplayName" "С�Ǻ����뷨"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Weasel" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Weasel" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Weasel" "NoRepair" 1
  WriteUninstaller "$INSTDIR\uninstall.exe"

SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\С�Ǻ����뷨"
  CreateShortCut "$SMPROGRAMS\С�Ǻ����뷨\��С�Ǻ���˵����.lnk" "$INSTDIR\README.txt"
  CreateShortCut "$SMPROGRAMS\С�Ǻ����뷨\��С�Ǻ������뷨�趨.lnk" "$INSTDIR\WeaselDeployer.exe" "" "$SYSDIR\shell32.dll" 21
  CreateShortCut "$SMPROGRAMS\С�Ǻ����뷨\��С�Ǻ����û��ʵ����.lnk" "$INSTDIR\WeaselDeployer.exe" "/dict" "$SYSDIR\shell32.dll" 6
  CreateShortCut "$SMPROGRAMS\С�Ǻ����뷨\��С�Ǻ����û�����ͬ��.lnk" "$INSTDIR\WeaselDeployer.exe" "/sync" "$SYSDIR\shell32.dll" 26
  CreateShortCut "$SMPROGRAMS\С�Ǻ����뷨\��С�Ǻ������²���.lnk" "$INSTDIR\WeaselDeployer.exe" "/deploy" "$SYSDIR\shell32.dll" 144
  CreateShortCut "$SMPROGRAMS\С�Ǻ����뷨\С�Ǻ��㷨����.lnk" "$INSTDIR\WeaselServer.exe" "" "$INSTDIR\WeaselServer.exe" 0
  CreateShortCut "$SMPROGRAMS\С�Ǻ����뷨\��С�Ǻ����û��ļ���.lnk" "$INSTDIR\WeaselServer.exe" "/userdir" "$SYSDIR\shell32.dll" 126
  CreateShortCut "$SMPROGRAMS\С�Ǻ����뷨\��С�Ǻ��������ļ���.lnk" "$INSTDIR\WeaselServer.exe" "/weaseldir" "$SYSDIR\shell32.dll" 19
  CreateShortCut "$SMPROGRAMS\С�Ǻ����뷨\��С�Ǻ�������°汾.lnk" "$INSTDIR\WeaselServer.exe" "/update" "$SYSDIR\shell32.dll" 13
  ${If} ${RunningX64}
    CreateShortCut "$SMPROGRAMS\С�Ǻ����뷨\��С�Ǻ�����װѡ��.lnk" "$INSTDIR\WeaselSetupx64.exe" "" "$SYSDIR\shell32.dll" 162
  ${Else}
    CreateShortCut "$SMPROGRAMS\С�Ǻ����뷨\��С�Ǻ�����װѡ��.lnk" "$INSTDIR\WeaselSetup.exe" "" "$SYSDIR\shell32.dll" 162
  ${EndIf}
  CreateShortCut "$SMPROGRAMS\С�Ǻ����뷨\ж��С�Ǻ�.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0

SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"

  ExecWait '"$INSTDIR\WeaselServer.exe" /quit'

  ${If} ${RunningX64}
    ExecWait '"$INSTDIR\WeaselSetupx64.exe" /u'
  ${Else}
    ExecWait '"$INSTDIR\WeaselSetup.exe" /u'
  ${EndIf}

  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Weasel"
  DeleteRegKey HKLM SOFTWARE\Rime

  ; Remove files and uninstaller
  SetOutPath $TEMP
  Delete /REBOOTOK "$INSTDIR\data\opencc\*.*"
  Delete /REBOOTOK "$INSTDIR\data\preview\*.*"
  Delete /REBOOTOK "$INSTDIR\data\*.*"
  Delete /REBOOTOK "$INSTDIR\*.*"
  RMDir /REBOOTOK "$INSTDIR\data\opencc"
  RMDir /REBOOTOK "$INSTDIR\data\preview"
  RMDir /REBOOTOK "$INSTDIR\data"
  Delete /REBOOTOK "$INSTDIR\������������Դ��\librime\include\rime\gear\*.*"
  RMDir /REBOOTOK "$INSTDIR\������������Դ��\librime\include\rime\gear"
  RMDir /REBOOTOK "$INSTDIR\������������Դ��\librime\include\rime"
  RMDir /REBOOTOK "$INSTDIR\������������Դ��\librime\include"
  Delete /REBOOTOK "$INSTDIR\������������Դ��\librime\src\gear\*.*"
  RMDir /REBOOTOK "$INSTDIR\������������Դ��\librime\src\gear"
  RMDir /REBOOTOK "$INSTDIR\������������Դ��\librime\src"
  RMDir /REBOOTOK "$INSTDIR\������������Դ��\librime"
  RMDir /REBOOTOK "$INSTDIR\������������Դ��"
  RMDir /REBOOTOK "$INSTDIR"
  SetShellVarContext all
  Delete /REBOOTOK "$SMPROGRAMS\С�Ǻ����뷨\*.*"
  RMDir /REBOOTOK "$SMPROGRAMS\С�Ǻ����뷨"

SectionEnd
