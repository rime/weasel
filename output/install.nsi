; weasel installation script
!include FileFunc.nsh
!include LogicLib.nsh
!include x64.nsh

!define WEASEL_VERSION 0.9.14
!define WEASEL_BUILD ${WEASEL_VERSION}.0

!define WEASEL_ROOT $INSTDIR\weasel-${WEASEL_VERSION}

; The name of the installer
Name "小狼毫 ${WEASEL_VERSION}"

; The file to write
OutFile "archives\weasel-${WEASEL_BUILD}-installer.exe"

VIProductVersion "${WEASEL_BUILD}"
VIAddVersionKey /LANG=2052 "ProductName" "小狼毫"
VIAddVersionKey /LANG=2052 "Comments" "Powered by RIME | 中州韻輸入法引擎"
VIAddVersionKey /LANG=2052 "CompanyName" "式恕堂"
VIAddVersionKey /LANG=2052 "LegalCopyright" "Copyleft RIME Developers 2012"
VIAddVersionKey /LANG=2052 "FileDescription" "小狼毫輸入法"
VIAddVersionKey /LANG=2052 "FileVersion" "${WEASEL_VERSION}"

Icon zhung.ico
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
 
  MessageBox MB_OKCANCEL|MB_ICONINFORMATION \
  "安裝前，我打盤先卸載舊版本的小狼毫。$\n$\n按下「確定」移除舊版本，按下「取消」放棄本次安裝。" \
  IDOK uninst
  Abort
 
; Run the uninstaller silently
uninst:
  Exec '$R0 /S'
done:

FunctionEnd

; The stuff to install
Section "Weasel"

  SectionIn RO
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\Rime\Weasel "InstallDir" "$INSTDIR"
  
  StrCpy $INSTDIR "${WEASEL_ROOT}"

  IfFileExists "$INSTDIR\WeaselServer.exe" 0 +2
  ExecWait '"$INSTDIR\WeaselServer.exe" /quit'

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  ; program files
  File "LICENSE.txt"
  File "README.txt"
  File "weasel.ime"
  File "weaselx64.ime"
  File "weaselt.ime"
  File "weaseltx64.ime"
  File "WeaselDeployer.exe"
  File "WeaselServer.exe"
  File "opencc.dll"
  File "WinSparkle.dll"
  File "zlib1.dll"
  ; shared data files
  SetOutPath  $INSTDIR\data
  File "data\default.yaml"
  File "data\weasel.yaml"
  File "data\essay.kct"
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

  SetOutPath $INSTDIR
  
  ; test /T flag for zh_TW locale
  StrCpy $R2  "weasel"
  ${GetParameters} $R0
  ClearErrors
  ${GetOptions} $R0 "/T" $R1
  IfErrors +2 0
  StrCpy $R2 "weaselt"

  ${If} ${RunningX64}
    ExecWait 'rundll32 "$INSTDIR\$R2x64.ime" install'
  ${Else}
    ExecWait 'rundll32 "$INSTDIR\$R2.ime" install'
  ${EndIf}

  ; run as user...
  ExecWait "$INSTDIR\WeaselDeployer.exe /install"

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Weasel" "DisplayName" "小狼毫輸入法"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Weasel" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Weasel" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Weasel" "NoRepair" 1
  WriteUninstaller "$INSTDIR\uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\小狼毫輸入法"
  CreateShortCut "$SMPROGRAMS\小狼毫輸入法\說明書.lnk" "$INSTDIR\README.txt"
  CreateShortCut "$SMPROGRAMS\小狼毫輸入法\輸入法設定.lnk" "$INSTDIR\WeaselDeployer.exe" "" "$INSTDIR\WeaselDeployer.exe" 0
  CreateShortCut "$SMPROGRAMS\小狼毫輸入法\用戶詞典管理.lnk" "$INSTDIR\WeaselDeployer.exe" "/dict" "$INSTDIR\WeaselDeployer.exe" 0
  CreateShortCut "$SMPROGRAMS\小狼毫輸入法\小狼毫算法服務.lnk" "$INSTDIR\WeaselServer.exe" "" "$INSTDIR\WeaselServer.exe" 0
  CreateShortCut "$SMPROGRAMS\小狼毫輸入法\卸載小狼毫.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ExecWait '"$INSTDIR\WeaselServer.exe" /quit'

  ${If} ${RunningX64}
    ExecWait 'rundll32 "$INSTDIR\weaselx64.ime" uninstall'
  ${Else}
    ExecWait 'rundll32 "$INSTDIR\weasel.ime" uninstall'
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
  RMDir /REBOOTOK "$INSTDIR"
  Delete /REBOOTOK "$SMPROGRAMS\小狼毫輸入法\*.*"
  RMDir /REBOOTOK "$SMPROGRAMS\小狼毫輸入法"

SectionEnd
