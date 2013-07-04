; weasel expansion installation script
!include FileFunc.nsh
!include LogicLib.nsh
!include x64.nsh

; 擴展包標識
!define PACKAGE_ID "weasel-expansion"
; 擴展包版本
!define PACKAGE_VERSION 0.9.18
; 必須補足 4 個整數
!define PACKAGE_BUILD ${PACKAGE_VERSION}.0
; 擴展包名稱
!define PACKAGE_NAME "小狼毫擴展方案集"

; The name of the installer
Name "${PACKAGE_NAME} ${PACKAGE_VERSION}"

; The file to write
OutFile "archives\${PACKAGE_ID}-${PACKAGE_BUILD}.exe"

VIProductVersion "${PACKAGE_BUILD}"
VIAddVersionKey /LANG=2052 "ProductName" "${PACKAGE_NAME}"
VIAddVersionKey /LANG=2052 "Comments" "Powered by RIME | 中州韻輸入法引擎"
VIAddVersionKey /LANG=2052 "CompanyName" "式恕堂"
VIAddVersionKey /LANG=2052 "LegalCopyright" "Copyleft RIME Developers"
VIAddVersionKey /LANG=2052 "FileDescription" "${PACKAGE_NAME}"
VIAddVersionKey /LANG=2052 "FileVersion" "${PACKAGE_VERSION}"

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

;--------------------------------

Function .onInit
  ; 讀取小狼毫安裝目錄
  ReadRegStr $R0 HKLM SOFTWARE\Rime\Weasel "WeaselRoot"
  StrCmp $R0 "" 0 done

  ; 找不到小狼毫安裝目錄，只好放這裏了
  StrCpy $R0 "$INSTDIR\${PACKAGE_ID}-${PACKAGE_VERSION}"

  MessageBox MB_OKCANCEL|MB_ICONINFORMATION \
  "找不到【小狼毫】安裝在哪裏。$\n$\n還要繼續安裝「${PACKAGE_NAME}」嗎？" \
  IDOK done
  Abort

done:
  ; Reset INSTDIR for the new version
  StrCpy $INSTDIR $R0
FunctionEnd

; The stuff to install
Section "Package"

  SectionIn RO

  IfFileExists "$INSTDIR\WeaselServer.exe" 0 +2
  ExecWait '"$INSTDIR\WeaselServer.exe" /quit'

  SetOverwrite try
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; shared data files
  SetOutPath  $INSTDIR\data
  File "expansion\*.schema.yaml"
  File "expansion\*.dict.yaml"
  ; We can even replace preset files with a newer/customized version
  ;File default.yaml
  ;File weasel.yaml

  ; opencc data files
  SetOutPath  $INSTDIR\data\opencc
  File "expansion\opencc\*.ini"
  File "expansion\opencc\*.ocd"
  ;File "expansion\opencc\*.txt"

  ; images
  ;SetOutPath  $INSTDIR\data\preview
  ;File "data\preview\*.png"
SectionEnd

;--------------------------------
