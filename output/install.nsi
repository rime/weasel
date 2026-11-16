; weasel installation script
!include FileFunc.nsh
!include LogicLib.nsh
!include MUI2.nsh
!include x64.nsh
!include winVer.nsh

Unicode true

;--------------------------------
; General

!ifndef WEASEL_VERSION
!define WEASEL_VERSION 0.1.0
!endif

!ifndef WEASEL_BUILD
!define WEASEL_BUILD 0
!endif

!define WEASEL_ROOT $INSTDIR\weasel-${WEASEL_VERSION}
!define REG_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Weasel"

; The name of the installer
Name "小狼毫 ${WEASEL_VERSION}"

; The file to write
OutFile "archives\weasel-${PRODUCT_VERSION}-installer.exe"

VIProductVersion "${WEASEL_VERSION}.${WEASEL_BUILD}"
VIAddVersionKey /LANG=2052 "ProductName" "小狼毫"
VIAddVersionKey /LANG=2052 "Comments" "Powered by RIME | 中州韻輸入法引擎"
VIAddVersionKey /LANG=2052 "CompanyName" "式恕堂"
VIAddVersionKey /LANG=2052 "LegalCopyright" "Copyleft RIME Developers"
VIAddVersionKey /LANG=2052 "FileDescription" "小狼毫輸入法"
VIAddVersionKey /LANG=2052 "FileVersion" "${WEASEL_VERSION}"

!define MUI_ICON ..\resource\weasel.ico
SetCompressor /SOLID lzma


; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------

; Pages

!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

;--------------------------------

; Languages

!insertmacro MUI_LANGUAGE "TradChinese"
LangString DISPLAYNAME ${LANG_TRADCHINESE} "小狼毫輸入法"
LangString LNKFORMANUAL ${LANG_TRADCHINESE} "【小狼毫】說明書"
LangString LNKFORSETTING ${LANG_TRADCHINESE} "【小狼毫】輸入法設定"
LangString LNKFORDICT ${LANG_TRADCHINESE} "【小狼毫】用戶詞典管理"
LangString LNKFORSYNC ${LANG_TRADCHINESE} "【小狼毫】用戶資料同步"
LangString LNKFORDEPLOY ${LANG_TRADCHINESE} "【小狼毫】重新部署"
LangString LNKFORSERVER ${LANG_TRADCHINESE} "小狼毫算法服務"
LangString LNKFORUSERFOLDER ${LANG_TRADCHINESE} "【小狼毫】用戶文件夾"
LangString LNKFORAPPFOLDER ${LANG_TRADCHINESE} "【小狼毫】程序文件夾"
LangString LNKFORUPDATER ${LANG_TRADCHINESE} "【小狼毫】檢查新版本"
LangString LNKFORSETUP ${LANG_TRADCHINESE} "【小狼毫】安裝選項"
LangString LNKFORUNINSTALL ${LANG_TRADCHINESE} "卸載小狼毫"
LangString CONFIRMATION ${LANG_TRADCHINESE} "安裝前，請先卸載舊版本的小狼毫。$\n$\n按下「確定」移除舊版本，按下「取消」放棄本次安裝。"
LangString SYSTEMVERSIONNOTOK ${LANG_TRADCHINESE} "您的系统不被支持，最低系統要求:Windows 8.1!"
LangString AUTOCHKUPDATE ${LANG_TRADCHINESE} "自動檢查版本更新？"

!insertmacro MUI_LANGUAGE "SimpChinese"
LangString DISPLAYNAME ${LANG_SIMPCHINESE} "小狼毫输入法"
LangString LNKFORMANUAL ${LANG_SIMPCHINESE} "【小狼毫】说明书"
LangString LNKFORSETTING ${LANG_SIMPCHINESE} "【小狼毫】输入法设定"
LangString LNKFORDICT ${LANG_SIMPCHINESE} "【小狼毫】用户词典管理"
LangString LNKFORSYNC ${LANG_SIMPCHINESE} "【小狼毫】用户资料同步"
LangString LNKFORDEPLOY ${LANG_SIMPCHINESE} "【小狼毫】重新部署"
LangString LNKFORSERVER ${LANG_SIMPCHINESE} "小狼毫算法服务"
LangString LNKFORUSERFOLDER ${LANG_SIMPCHINESE} "【小狼毫】用户文件夹"
LangString LNKFORAPPFOLDER ${LANG_SIMPCHINESE} "【小狼毫】程序文件夹"
LangString LNKFORUPDATER ${LANG_SIMPCHINESE} "【小狼毫】检查新版本"
LangString LNKFORSETUP ${LANG_SIMPCHINESE} "【小狼毫】安装选项"
LangString LNKFORUNINSTALL ${LANG_SIMPCHINESE} "卸载小狼毫"
LangString CONFIRMATION ${LANG_SIMPCHINESE} '安装前，请先卸载旧版本的小狼毫。$\n$\n点击 "确定" 移除旧版本，或点击 "取消" 放弃本次安装。'
LangString SYSTEMVERSIONNOTOK ${LANG_SIMPCHINESE} "您的系統不被支持，最低系统要求:Windows 8.1!"
LangString AUTOCHKUPDATE ${LANG_SIMPCHINESE} "自动检查版本更新？"

!insertmacro MUI_LANGUAGE "English"
LangString DISPLAYNAME ${LANG_ENGLISH} "Weasel"
LangString LNKFORMANUAL ${LANG_ENGLISH} "Weasel Manual"
LangString LNKFORSETTING ${LANG_ENGLISH} "Weasel Settings"
LangString LNKFORDICT ${LANG_ENGLISH} "Weasel Dictionary Manager"
LangString LNKFORSYNC ${LANG_ENGLISH} "Weasel Sync User Profile"
LangString LNKFORDEPLOY ${LANG_ENGLISH} "Weasel Deploy"
LangString LNKFORSERVER ${LANG_ENGLISH} "Weasel Server"
LangString LNKFORUSERFOLDER ${LANG_ENGLISH} "Weasel User Folder"
LangString LNKFORAPPFOLDER ${LANG_ENGLISH} "Weasel App Folder"
LangString LNKFORUPDATER ${LANG_ENGLISH} "Weasel Check for Updates"
LangString LNKFORSETUP ${LANG_ENGLISH} "Weasel Installation Preference"
LangString LNKFORUNINSTALL ${LANG_ENGLISH} "Uninstall Weasel"
LangString CONFIRMATION ${LANG_ENGLISH} "Before installation, please uninstall the old version of Weasel.$\n$\nPress 'OK' to remove the old version, or 'Cancel' to abort installation."
LangString SYSTEMVERSIONNOTOK ${LANG_ENGLISH} "Your system not supported, minimium system required: Windows 8.1!"
LangString AUTOCHKUPDATE ${LANG_ENGLISH} "Automatically check for updates?"
LangString SERVERSTILLRUNNING ${LANG_TRADCHINESE} "小狼毫算法服務無法結束，安裝已中止。$\n$\n請在工作管理員手動結束 WeaselServer.exe 後重新執行安裝程式。"
LangString SERVERSTILLRUNNING ${LANG_SIMPCHINESE} "小狼毫算法服务无法结束，安装已中止。$\n$\n请在任务管理器手动结束 WeaselServer.exe 后重新运行安装程序。"
LangString SERVERSTILLRUNNING ${LANG_ENGLISH} "WeaselServer.exe could not be stopped; installation aborted.$\n$\nPlease end WeaselServer.exe manually in Task Manager and run the installer again."
LangString STALEBINARIES ${LANG_TRADCHINESE} "偵測到舊版檔案未被替換（仍被鎖定）：$\n$\n$R2$\n$\n安裝已中止，以避免半安裝狀態。請結束佔用這些檔案的程式後重試。"
LangString STALEBINARIES ${LANG_SIMPCHINESE} "检测到旧版文件未被替换（仍被锁定）：$\n$\n$R2$\n$\n安装已中止，以避免半安装状态。请结束占用这些文件的程序后重试。"
LangString STALEBINARIES ${LANG_ENGLISH} "Stale files could not be replaced (still locked):$\n$\n$R2$\n$\nInstallation aborted to avoid a half-installed state. Close the programs using these files and retry."

;--------------------------------

; StopWeaselServer <serverExePath>
;   Ask the running server to quit, then make sure the process is really
;   gone. A half-quit server keeps WeaselServer.exe / rime.dll /
;   WinSparkle.dll locked, which turned past upgrades into silent
;   half-installs (stale binaries under a new DisplayVersion), so /quit
;   alone is not enough.
;
;   Implementation notes:
;   - NSIS cannot wait on an already-running process handle, so poll with a
;     lock probe: a running WeaselServer.exe cannot be renamed; success
;     means the process has exited. Undo the probe immediately.
;   - If the server is hung (it crashed hosts in the wild and may not
;     answer /quit), force-kill via taskkill after a bounded graceful wait.
;     taskkill is used instead of the nsProcess plugin to avoid adding a
;     plugin dependency to the CI build.
;   - Only registry-free, bundled tools are used; no extra downloads.
!macro StopWeaselServer SERVER_EXE
  Push $R0
  Push $R1
  Push $R2
  StrCpy $R1 "${SERVER_EXE}"
  ${If} ${FileExists} "$R1"
    ExecWait '"$R1" /quit'
    StrCpy $R2 0
  wait_quit_loop:
    Sleep 500
    IntOp $R2 $R2 + 1
    ClearErrors
    Rename "$R1" "$R1.weasel-lockprobe"
    ${If} ${Errors}
      ${If} $R2 < 20 ; bounded graceful wait: ~10s
        Goto wait_quit_loop
      ${EndIf}
      ; Still locked: force kill, then one more bounded wait.
      nsExec::Exec 'taskkill /IM WeaselServer.exe /F'
      Pop $R0
      StrCpy $R2 0
    force_wait_loop:
      Sleep 500
      IntOp $R2 $R2 + 1
      ClearErrors
      Rename "$R1" "$R1.weasel-lockprobe"
      ${If} ${Errors}
        ${If} $R2 < 6 ; ~3s after force kill
          Goto force_wait_loop
        ${EndIf}
        ; Server could not be stopped: abort, never half-install.
        SetErrors
        IfSilent server_abort_silent_${__LINE__}
        MessageBox MB_OK|MB_ICONSTOP "$(SERVERSTILLRUNNING)"
        server_abort_silent_${__LINE__}:
        Pop $R2
        Pop $R1
        Pop $R0
        Abort "$(SERVERSTILLRUNNING)"
      ${EndIf}
    ${EndIf}
    ; Probe succeeded: the process is gone; restore the original name so the
    ; following upgrade/uninstall logic sees an intact file layout.
    ClearErrors
    Rename "$R1.weasel-lockprobe" "$R1"
    ${If} ${Errors}
      ; Extremely unlikely (rename inside same dir). Old name will be
      ; replaced by File below anyway if we are installing; for uninstall
      ; the following Delete will drop the probe file instead.
      Delete /REBOOTOK "$R1.weasel-lockprobe"
    ${EndIf}
  ${EndIf}
  Pop $R2
  Pop $R1
  Pop $R0
!macroend

; VerifyFreshBinaries
;   Run right after the File copies. SetOverwrite try silently skips locked
;   files, so assert that the critical binaries on disk really match this
;   build before the installer claims success in the registry.
;
;   Mechanism: rename-probe each critical binary right after copying. A file
;   still held open by a live process cannot be renamed, so a failed probe
;   marks it stale. We do not compare versions or sizes: WeaselServer.exe /
;   rime.dll / WinSparkle.dll ship without a usable version resource
;   (observed on the 0.17.4 build), and same-size-same-version stale copies
;   are exactly what a locked-file skip leaves behind.
;   The probe is undone immediately; a successful rename proves nothing is
;   holding the file, and the copy above already delivered new content.
!macro VerifyFreshBinaries
  Push $R0
  Push $R3
  StrCpy $R3 ""
  StrCpy $R0 "WeaselServer.exe"
  Call VerifyOneBinary
  StrCpy $R0 "weasel.dll"
  Call VerifyOneBinary
  StrCpy $R0 "weaselx64.dll"
  Call VerifyOneBinary
  StrCpy $R0 "rime.dll"
  Call VerifyOneBinary
  StrCpy $R0 "WeaselDeployer.exe"
  Call VerifyOneBinary
  ${If} $R3 != ""
    StrCpy $R2 $R3
    IfSilent verify_fail_silent
    MessageBox MB_OK|MB_ICONSTOP "$(STALEBINARIES)"
    verify_fail_silent:
    Pop $R3
    Pop $R0
    Abort "$(STALEBINARIES)"
  ${EndIf}
  Pop $R3
  Pop $R0
!macroend

; Helper called with $R0 = file name; appends stale entries to $R3.
Function VerifyOneBinary
  Push $R4
  ${IfNot} ${FileExists} "$INSTDIR\$R0"
    Goto verify_one_done
  ${EndIf}
  ClearErrors
  Rename "$INSTDIR\$R0" "$INSTDIR\$R0.weasel-lockprobe"
  ${If} ${Errors}
    ; Rename failed: the file is still locked by a live process, i.e. the
    ; copy above could not have replaced it. Record and continue so the
    ; dialog lists every stale file at once.
    StrCpy $R3 "$R3$\n$INSTDIR\$R0"
    Goto verify_one_nofix
  ${EndIf}
  ; Probe succeeded: nothing holds the file. Restore the name the install
  ; layout expects (content is already the fresh copy).
  ClearErrors
  Rename "$INSTDIR\$R0.weasel-lockprobe" "$INSTDIR\$R0"
  ${If} ${Errors}
    ; Extremely unlikely (rename inside the same directory): force the
    ; restore at reboot rather than leaving a stray probe file.
    Delete /REBOOTOK "$INSTDIR\$R0.weasel-lockprobe"
  ${EndIf}
  verify_one_nofix:
  verify_one_done:
  Pop $R4
FunctionEnd

Function .onInit
  ; if not version >= 8.1, quit and MessageBox(if not silent)
  ${IfNot} ${AtLeastWin8.1}
    IfSilent toquit
    MessageBox MB_OK '$(SYSTEMVERSIONNOTOK)'
toquit:
    Quit
  ${EndIf}

  ReadRegStr $R0 HKLM "Software\Rime\Weasel" "InstallDir"
  StrCmp $R0 "" 0 skip
  ; The default installation directory
  ; install x64 build for NativeARM64_WINDOWS11 and NativeAMD64_WINDOWS11
  ${If} ${AtLeastWin11} ; Windows 11 and above
    ${If} ${IsNativeARM64}
      StrCpy $INSTDIR "$PROGRAMFILES64\Rime"
    ${ElseIf} ${IsNativeAMD64}
      StrCpy $INSTDIR "$PROGRAMFILES64\Rime"
    ${Else}
      StrCpy $INSTDIR "$PROGRAMFILES\Rime"
    ${Endif}
  ; install x64 build for NativeAMD64_BELLOW_WINDOWS11
  ${Else} ; Windows 10 or bellow
    ${If} ${IsNativeAMD64}
      StrCpy $INSTDIR "$PROGRAMFILES64\Rime"
    ${Else}
      StrCpy $INSTDIR "$PROGRAMFILES\Rime"
    ${Endif}
  ${Endif}
skip:
  ReadRegStr $R0 HKLM \
  "Software\Microsoft\Windows\CurrentVersion\Uninstall\Weasel" \
  "UninstallString"
  StrCmp $R0 "" done

  StrCpy $0 "Upgrade"
  IfSilent uninst 0
  MessageBox MB_OKCANCEL|MB_ICONINFORMATION "$(CONFIRMATION)" IDOK uninst
  Abort

uninst:
  ; Backup data directory from previous installation, user files may exist
  ReadRegStr $R1 HKLM SOFTWARE\Rime\Weasel "WeaselRoot"
  StrCmp $R1 "" call_uninstaller
  IfFileExists $R1\data\*.* 0 call_uninstaller
  CreateDirectory $TEMP\weasel-backup
  CopyFiles $R1\data\*.* $TEMP\weasel-backup

call_uninstaller:
  ; Wait until the old server is really gone before deleting files; a
  ; half-quit server leaves locked binaries behind (stale-file cause).
  !insertmacro StopWeaselServer "$R1\WeaselServer.exe"
  ; Keep TSF profiles during upgrades so Windows does not add a default IME.
  ; Remove registry keys
  DeleteRegKey HKLM SOFTWARE\Rime
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Weasel"
  ; don't redirect on 64 bit system for auto run setting
  ${If} ${IsNativeARM64}
    SetRegView 64
  ${ElseIf} ${IsNativeAMD64}
    SetRegView 64
  ${Endif}
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "WeaselServer"
  ; recover back to 32bit view
  SetRegView 32
  ; Remove files and uninstaller
  Delete  "$R1\data\opencc\*.*"
  Delete  "$R1\data\*.*"
  Delete  "$R1\*.*"
  RMDir   "$R1\data\opencc"
  RMDir   "$R1\data"
  RMDir   "$R1"
  SetShellVarContext all
  Delete  "$SMPROGRAMS\$(DISPLAYNAME)\*.*"
  RMDir  "$SMPROGRAMS\$(DISPLAYNAME)"
  ; Prompt reboot
  SetRebootFlag true
  Sleep 800

done:
FunctionEnd

; Registry key to check for directory (so if you install again, it will
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\Rime\Weasel" "InstallDir"

; The stuff to install
Section "Weasel"

  SectionIn RO

  ; Write the new installation path into the registry
  ; redirect on 64 bit system
  ; HKLM SOFTWARE\WOW6432Node\Rime\Weasel "InstallDir" "$INSTDIR"
  WriteRegStr HKLM SOFTWARE\Rime\Weasel "InstallDir" "$INSTDIR"

  ; Reset INSTDIR for the new version
  StrCpy $INSTDIR "${WEASEL_ROOT}"

  ; Stop any server left running, including on Upgrade. The old skip-on-
  ; Upgrade path let a live server lock the binaries and silently turned
  ; the copies below into a half-install.
  ${If} ${FileExists} "$INSTDIR\WeaselServer.exe"
    !insertmacro StopWeaselServer "$INSTDIR\WeaselServer.exe"
  ${EndIf}

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
  File "7-zip-license.txt"
  File "7z.dll"
  File "7z.exe"
  File "COPYING-curl.txt"
  File "curl.exe"
  File "curl-ca-bundle.crt"
  File "rime-install.bat"
  File "rime-install-config.bat"
  File "start_service.bat"
  File "stop_service.bat"
  File "weasel.dll"
  ${If} ${RunningX64}
    File "weaselx64.dll"
  ${EndIf}
  ${If} ${IsNativeARM64}
    File /nonfatal "weaselARM.dll"
    File /nonfatal "weaselARM64.dll"
    File /nonfatal "weaselARM64X.dll"
  ${EndIf}
  ; install x64 build for NativeARM64_WINDOWS11 and NativeAMD64_WINDOWS11
  ${If} ${AtLeastWin11} ; Windows 11 and above
    ${If} ${IsNativeARM64}
      File "WeaselDeployer.exe"
      File "WeaselServer.exe"
      File "rime.dll"
      File "WinSparkle.dll"
    ${ElseIf} ${IsNativeAMD64}
      File "WeaselDeployer.exe"
      File "WeaselServer.exe"
      File "rime.dll"
      File "WinSparkle.dll"
    ${Else}
      File "Win32\WeaselDeployer.exe"
      File "Win32\WeaselServer.exe"
      File "Win32\rime.dll"
      File "Win32\WinSparkle.dll"
    ${Endif}
  ; install x64 build for NativeAMD64_BELLOW_WINDOWS11
  ${Else} ; Windows 10 or bellow
    ${If} ${IsNativeAMD64}
      File "WeaselDeployer.exe"
      File "WeaselServer.exe"
      File "rime.dll"
      File "WinSparkle.dll"
    ${Else}
      File "Win32\WeaselDeployer.exe"
      File "Win32\WeaselServer.exe"
      File "Win32\rime.dll"
      File "Win32\WinSparkle.dll"
    ${Endif}
  ${Endif}

  File "WeaselSetup.exe"
  File "WeaselLanguageProfile.ps1"
  ; shared data files
  SetOutPath $INSTDIR\data
  File "data\*.yaml"
  File /nonfatal "data\*.txt"
  File /nonfatal "data\*.gram"
  ; opencc data files
  SetOutPath $INSTDIR\data\opencc
  File "data\opencc\*.json"
  File "data\opencc\*.ocd*"

  SetOutPath $INSTDIR

  ; Assert the critical binaries were really replaced before the registry
  ; below claims the new version. A locked file skipped by SetOverwrite try
  ; would otherwise leave stale binaries under a fresh DisplayVersion.
  !insertmacro VerifyFreshBinaries

  ; test /T flag for zh_TW locale
  StrCpy $R2 "/i"
  ${GetParameters} $R0
  ClearErrors
  ${GetOptions} $R0 "/S" $R1
  IfErrors +2 0
  StrCpy $R2 "/s"
  ${GetOptions} $R0 "/T" $R1
  IfErrors +2 0
  StrCpy $R2 "/t"

  ExecWait '"$INSTDIR\WeaselSetup.exe" $R2'

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "${REG_UNINST_KEY}" "DisplayName" "$(DISPLAYNAME)"
  WriteRegStr HKLM "${REG_UNINST_KEY}" "DisplayIcon" '"$INSTDIR\WeaselServer.exe"'
  WriteRegStr HKLM "${REG_UNINST_KEY}" "DisplayVersion" "${WEASEL_VERSION}.${WEASEL_BUILD}"
  WriteRegStr HKLM "${REG_UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "${REG_UNINST_KEY}" "Publisher" "式恕堂"
  WriteRegStr HKLM "${REG_UNINST_KEY}" "URLInfoAbout" "https://rime.im/"
  WriteRegStr HKLM "${REG_UNINST_KEY}" "HelpLink" "https://rime.im/docs/"
  WriteRegDWORD HKLM "${REG_UNINST_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${REG_UNINST_KEY}" "NoRepair" 1
  WriteUninstaller "$INSTDIR\uninstall.exe"

  ; run as user...
  IfSilent deploy_silently
  ExecWait "$INSTDIR\WeaselDeployer.exe /install"
  GoTo deploy_done

  deploy_silently:
  ExecWait "$INSTDIR\WeaselDeployer.exe /deploy"
  deploy_done:

  ; don't redirect on 64 bit system for auto run setting
  ${If} ${IsNativeARM64}
    SetRegView 64
  ${ElseIf} ${IsNativeAMD64}
    SetRegView 64
  ${Endif}
  ; Write autorun key
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "WeaselServer" "$INSTDIR\WeaselServer.exe"
  ; Start WeaselServer
  Exec "$INSTDIR\WeaselServer.exe"

  ; option CheckForUpdates
  IfSilent DisableAutoCheckUpdate
  MessageBox MB_YESNO|MB_ICONINFORMATION "$(AUTOCHKUPDATE)" IDYES EnableAutoCheckUpdate
  DisableAutoCheckUpdate:
  WriteRegStr HKCU "Software\Rime\Weasel\Updates" "CheckForUpdates" "0"
  GoTo end
  EnableAutoCheckUpdate:
  WriteRegStr HKCU "Software\Rime\Weasel\Updates" "CheckForUpdates" "1"
  end:

  ; Prompt reboot
  StrCmp $0 "Upgrade" 0 +2
  SetRebootFlag true

SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\$(DISPLAYNAME)"
  CreateShortCut "$SMPROGRAMS\$(DISPLAYNAME)\$(LNKFORMANUAL).lnk" "$INSTDIR\README.txt"
  CreateShortCut "$SMPROGRAMS\$(DISPLAYNAME)\$(LNKFORSETTING).lnk" "$INSTDIR\WeaselDeployer.exe" "" "$SYSDIR\shell32.dll" 21
  CreateShortCut "$SMPROGRAMS\$(DISPLAYNAME)\$(LNKFORDICT).lnk" "$INSTDIR\WeaselDeployer.exe" "/dict" "$SYSDIR\shell32.dll" 6
  CreateShortCut "$SMPROGRAMS\$(DISPLAYNAME)\$(LNKFORSYNC).lnk" "$INSTDIR\WeaselDeployer.exe" "/sync" "$SYSDIR\shell32.dll" 26
  CreateShortCut "$SMPROGRAMS\$(DISPLAYNAME)\$(LNKFORDEPLOY).lnk" "$INSTDIR\WeaselDeployer.exe" "/deploy" "$SYSDIR\shell32.dll" 144
  CreateShortCut "$SMPROGRAMS\$(DISPLAYNAME)\$(LNKFORSERVER).lnk" "$INSTDIR\WeaselServer.exe" "" "$INSTDIR\WeaselServer.exe" 0
  CreateShortCut "$SMPROGRAMS\$(DISPLAYNAME)\$(LNKFORUSERFOLDER).lnk" "$INSTDIR\WeaselServer.exe" "/userdir" "$SYSDIR\shell32.dll" 126
  CreateShortCut "$SMPROGRAMS\$(DISPLAYNAME)\$(LNKFORAPPFOLDER).lnk" "$INSTDIR\WeaselServer.exe" "/weaseldir" "$SYSDIR\shell32.dll" 19
  CreateShortCut "$SMPROGRAMS\$(DISPLAYNAME)\$(LNKFORUPDATER).lnk" "$INSTDIR\WeaselServer.exe" "/update" "$SYSDIR\shell32.dll" 13
  CreateShortCut "$SMPROGRAMS\$(DISPLAYNAME)\$(LNKFORSETUP).lnk" "$INSTDIR\WeaselSetup.exe" "" "$SYSDIR\shell32.dll" 162
  CreateShortCut "$SMPROGRAMS\$(DISPLAYNAME)\$(LNKFORUNINSTALL).lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0

SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"

  ; Same anti-half-install guarantee for removal: wait until the server is
  ; really gone before deleting, else locked files survive the uninstall.
  !insertmacro StopWeaselServer "$INSTDIR\WeaselServer.exe"

  ExecWait '"$INSTDIR\WeaselSetup.exe" /u'

  ; Remove registry keys
  DeleteRegKey HKLM SOFTWARE\Rime
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Weasel"
  ; don't redirect on 64 bit system for auto run setting
  ${If} ${IsNativeARM64}
    SetRegView 64
  ${ElseIf} ${IsNativeAMD64}
    SetRegView 64
  ${Endif}
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "WeaselServer"

  ; Remove files and uninstaller
  SetOutPath $TEMP
  Delete  "$INSTDIR\data\opencc\*.*"
  Delete  "$INSTDIR\data\*.*"
  Delete  "$INSTDIR\*.*"
  RMDir  "$INSTDIR\data\opencc"
  RMDir  "$INSTDIR\data"
  RMDir  "$INSTDIR"
  SetShellVarContext all
  Delete  "$SMPROGRAMS\$(DISPLAYNAME)\*.*"
  RMDir  "$SMPROGRAMS\$(DISPLAYNAME)"

  ; Prompt reboot
  SetRebootFlag true

SectionEnd
