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
LangString STALEBINARIES ${LANG_TRADCHINESE} "以下關鍵檔案未能安裝（寫入失敗或被鎖定）：$\n$\n$R2$\n$\n安裝已中止，以避免半安裝狀態。請結束佔用這些檔案的程式後重試。"
LangString STALEBINARIES ${LANG_SIMPCHINESE} "以下关键文件未能安装（写入失败或被锁定）：$\n$\n$R2$\n$\n安装已中止，以避免半安装状态。请结束占用这些文件的程序后重试。"
LangString STALEBINARIES ${LANG_ENGLISH} "These critical files could not be installed (write failed or file locked):$\n$\n$R2$\n$\nInstallation aborted to avoid a half-installed state. Close the programs using these files and retry."
LangString SETUPFAILED ${LANG_TRADCHINESE} "WeaselSetup.exe 註冊輸入法失敗，安裝已中止。$\n$\n請結束所有使用小狼毫的程式（或重新開機）後重試。"
LangString SETUPFAILED ${LANG_SIMPCHINESE} "WeaselSetup.exe 注册输入法失败，安装已中止。$\n$\n请结束所有使用小狼毫的程序（或重启）后重试。"
LangString SETUPFAILED ${LANG_ENGLISH} "WeaselSetup.exe failed to register the input method; installation aborted.$\n$\nClose all programs using Weasel (or reboot) and retry."
LangString STALEIMEDLLS ${LANG_TRADCHINESE} "系統目錄中的輸入法 DLL 未更新（仍為舊版）：$\n$\n$R2$\n$\n安裝已中止，以避免半安裝狀態。請結束使用這些 DLL 的程式後重試。"
LangString STALEIMEDLLS ${LANG_SIMPCHINESE} "系统目录中的输入法 DLL 未更新（仍为旧版）：$\n$\n$R2$\n$\n安装已中止，以避免半安装状态。请结束使用这些 DLL 的程序后重试。"
LangString STALEIMEDLLS ${LANG_ENGLISH} "The input method DLLs in the system directory were not updated (still the old build):$\n$\n$R2$\n$\nInstallation aborted to avoid a half-installed state. Close the programs using these DLLs and retry."

;--------------------------------

; FileCritical <source>
;   Extract one critical binary and record it when the write fails.
;   SetOverwrite is "on" around this block: a locked destination is then
;   reported instead of being skipped silently, and an interactive install
;   offers Retry so the user can close the locking program and continue.
;   ("try" skips without any prompt; it sets the error flag too, but it never
;   gives the user a chance to fix the lock mid-install - the old script did
;   not check the flag either way, which is what produced the half-installs
;   this PR fixes.) The error flag is checked right after each File because a
;   later command would otherwise mask an earlier failure.
!macro FileCritical SRC
  ClearErrors
  File "${SRC}"
  ${If} ${Errors}
    StrCpy $R3 "$R3$\n${SRC}"
  ${EndIf}
!macroend

; FileCriticalNonfatal <source>
;   Same as FileCritical for files the build may legitimately omit (the ARM
;   DLLs on a non-arm64 build): /nonfatal turns a missing source into a
;   compile warning, and a write failure of a packaged file is still
;   recorded.
!macro FileCriticalNonfatal SRC
  ClearErrors
  File /nonfatal "${SRC}"
  ${If} ${Errors}
    StrCpy $R3 "$R3$\n${SRC}"
  ${EndIf}
!macroend

;--------------------------------

; StopWeaselServer
;   Input: $R1 = full path to WeaselServer.exe; all other registers preserved.
;   Ask the running server to quit, then make sure the process is really
;   gone. A half-quit server keeps WeaselServer.exe / rime.dll /
;   WinSparkle.dll locked, which turned past upgrades into silent
;   half-installs (stale binaries under a new DisplayVersion), so /quit
;   alone is not enough.
;
;   Implementation notes:
;   - The graceful request is launched with `Exec`, NOT `ExecWait`:
;     WeaselServer.exe /quit is itself a client that connects to the running
;     server through a synchronous named-pipe IPC. PipeChannel::_Connect()
;     loops in WaitNamedPipe() with no overall deadline, and ShutdownServer()
;     blocks on a response with no timeout. If the server's pipe is stuck,
;     the /quit client hangs, and ExecWait would hang the installer behind it
;     before the helper below ever gets a chance to enforce its deadline.
;     Exec starts the request and returns immediately; only the helper below
;     waits, and it always reaches the force-kill and final check.
;   - Existence is polled by image path, NOT by a file lock probe: Windows
;     lets a running executable's file be renamed (only writes and deletes
;     are denied), so "rename succeeded" does not prove the process exited.
;     The helper script below waits up to 10s for the path-matched processes
;     to disappear, force-kills them if they do not, waits up to 3s more,
;     and exits 0 only when none is left; its exit code is the verdict.
;   - Every match is filtered by full image path: `taskkill /IM
;     WeaselServer.exe` would also terminate a Weasel installation living in
;     another directory.
;   - One body, two functions: NSIS compiles the installer and the
;     uninstaller as separate programs, and uninstaller code may only call
;     un.-prefixed functions (calling a plain one is a compile error), so
;     the uninstaller gets its own copy. Labels are function-scoped, so the
;     two copies cannot collide.
!macro StopWeaselServerDef UN
Function ${UN}StopWeaselServer
  Push $R0
  Push $R2
  ${If} ${FileExists} "$R1"
    InitPluginsDir
    FileOpen $R0 "$PLUGINSDIR\stop-weasel.ps1" w
    FileWrite $R0 "param([string]$$Path)$\r$\n"
    FileWrite $R0 "$$ErrorActionPreference='Stop'$\r$\n"
    FileWrite $R0 "function Get-Self { @(Get-CimInstance Win32_Process -Filter $\"Name='WeaselServer.exe'$\" | Where-Object { $$_.ExecutablePath -ieq $$Path }) }$\r$\n"
    FileWrite $R0 "$$d=(Get-Date).AddSeconds(10)$\r$\n"
    FileWrite $R0 "while((Get-Date) -lt $$d){ if((Get-Self).Count -eq 0){ exit 0 }; Start-Sleep -Milliseconds 250 }$\r$\n"
    FileWrite $R0 "Get-Self | ForEach-Object { Stop-Process -Id $$_.ProcessId -Force -ErrorAction SilentlyContinue }$\r$\n"
    FileWrite $R0 "$$d=(Get-Date).AddSeconds(3)$\r$\n"
    FileWrite $R0 "while((Get-Date) -lt $$d){ if((Get-Self).Count -eq 0){ exit 0 }; Start-Sleep -Milliseconds 250 }$\r$\n"
    FileWrite $R0 "exit 1$\r$\n"
    FileClose $R0
    ; Fire-and-forget: the helper below owns the whole timeout budget. Never
    ; ExecWait here - a /quit client stuck in WaitNamedPipe() would otherwise
    ; block this installer forever, so the force-kill below would never run.
    Exec '"$R1" /quit'
    nsExec::ExecToStack 'powershell -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File "$PLUGINSDIR\stop-weasel.ps1" -Path "$R1"'
    Pop $R0 ; exit code: 0 = no path-matched process left
    Pop $R2 ; captured output (unused)
    ${If} $R0 != "0"
      ; Server could not be stopped: abort, never half-install.
      IfSilent server_abort_silent
      MessageBox MB_OK|MB_ICONSTOP "$(SERVERSTILLRUNNING)"
      server_abort_silent:
      Pop $R2
      Pop $R0
      Abort "$(SERVERSTILLRUNNING)"
    ${EndIf}
  ${EndIf}
  Pop $R2
  Pop $R0
FunctionEnd
!macroend

!insertmacro StopWeaselServerDef ""
!insertmacro StopWeaselServerDef "un."

; VerifyCriticalBinaries
;   Called after the File block. $R3 already collects every critical File
;   that failed to write (see !macro FileCritical); this function adds the
;   ones that are simply absent, then aborts if anything is listed. A
;   missing critical binary must never pass silently - the old code skipped
;   absent files, so a file that was never installed looked like success.
Function VerifyCriticalBinaries
  Push $R0
  Push $R2
  Push $R3
  StrCpy $R0 "WeaselServer.exe"
  Call CheckOneCritical
  StrCpy $R0 "WeaselDeployer.exe"
  Call CheckOneCritical
  StrCpy $R0 "rime.dll"
  Call CheckOneCritical
  StrCpy $R0 "WinSparkle.dll"
  Call CheckOneCritical
  StrCpy $R0 "weasel.dll"
  Call CheckOneCritical
  ${If} ${RunningX64}
    StrCpy $R0 "weaselx64.dll"
    Call CheckOneCritical
  ${EndIf}
  ${If} $R3 != ""
    StrCpy $R2 $R3
    IfSilent verify_fail_silent
    MessageBox MB_OK|MB_ICONSTOP "$(STALEBINARIES)"
    verify_fail_silent:
    Pop $R3
    Pop $R2
    Pop $R0
    Abort "$(STALEBINARIES)"
  ${EndIf}
  Pop $R3
  Pop $R2
  Pop $R0
FunctionEnd

; Helper called with $R0 = file name; appends a missing-file entry to $R3.
; Already-recorded write failures are left alone; this only adds absence.
Function CheckOneCritical
  ${IfNot} ${FileExists} "$INSTDIR\$R0"
    StrCpy $R3 "$R3$\n$INSTDIR\$R0 (missing)"
  ${EndIf}
FunctionEnd

; VerifySystemImeDlls
;   WeaselSetup copies the IME DLLs into the system directories; those are the
;   copies host processes actually load, so a stale one is a half-install the
;   $INSTDIR probe cannot see. Compare against the freshly installed sources
;   by size (the x64 and 32-bit builds differ in size, so this distinguishes
;   builds without relying on version resources, which these DLLs lack).
;   A 32-bit installer is redirected away from System32, hence Sysnative.
Function VerifySystemImeDlls
  Push $R0
  Push $R1
  Push $R2
  Push $R3
  StrCpy $R3 ""
  ${If} ${RunningX64}
    ; Sysnative is the 32-bit installer's view of the real System32
    ; (FS redirection would otherwise send it to SysWOW64).
    StrCpy $R0 "$WINDIR\Sysnative\weasel.dll"
    StrCpy $R1 "$INSTDIR\weaselx64.dll"
    Call VerifyOneSystemIme
    StrCpy $R0 "$WINDIR\SysWOW64\weasel.dll"
    StrCpy $R1 "$INSTDIR\weasel.dll"
    Call VerifyOneSystemIme
  ${EndIf}
  ${If} $R3 != ""
    StrCpy $R2 $R3
    IfSilent sysime_fail_silent
    MessageBox MB_OK|MB_ICONSTOP "$(STALEIMEDLLS)"
    sysime_fail_silent:
    Pop $R3
    Pop $R2
    Pop $R1
    Pop $R0
    Abort "$(STALEIMEDLLS)"
  ${EndIf}
  Pop $R3
  Pop $R2
  Pop $R1
  Pop $R0
FunctionEnd

; Helper: $R0 = system copy, $R1 = freshly installed source.
; Appends a description to $R3 when the system copy is missing or differs.
Function VerifyOneSystemIme
  Push $9
  Push $R5
  Push $R6
  ClearErrors
  FileOpen $9 "$R1" r
  ${If} ${Errors}
    ; No such source (ARM64 layouts name files differently): nothing to check.
    Goto verify_sys_done
  ${EndIf}
  FileSeek $9 0 END $R5
  FileClose $9
  ClearErrors
  FileOpen $9 "$R0" r
  ${If} ${Errors}
    StrCpy $R3 "$R3$\n$R0 (missing)"
    Goto verify_sys_done
  ${EndIf}
  FileSeek $9 0 END $R6
  FileClose $9
  ${If} $R5 != $R6
    StrCpy $R3 "$R3$\n$R0 ($R6 bytes != $R5 bytes)"
  ${EndIf}
  verify_sys_done:
  Pop $R6
  Pop $R5
  Pop $9
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
  Push $R1
  StrCpy $R1 "$R1\WeaselServer.exe"
  Call StopWeaselServer
  Pop $R1
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
    Push $R1
    StrCpy $R1 "$INSTDIR\WeaselServer.exe"
    Call StopWeaselServer
    Pop $R1
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
  ; Critical binaries: a live server or a host process can hold these, so
  ; every extraction is checked immediately. SetOverwrite on (not try) makes
  ; NSIS raise the error flag instead of silently skipping a locked file.
  ; A successful File means these exact bytes were written, so success here
  ; is also the provenance guarantee - there is nothing left to hash.
  StrCpy $R3 ""
  SetOverwrite on
  !insertmacro FileCritical "weasel.dll"
  ${If} ${RunningX64}
    !insertmacro FileCritical "weaselx64.dll"
  ${EndIf}
  ${If} ${IsNativeARM64}
    ; ARM64-only layouts name files differently; on other arches these files
    ; are simply not packaged, so /nonfatal. When they ARE packaged, a write
    ; failure is still recorded like any other critical binary.
    !insertmacro FileCriticalNonfatal "weaselARM.dll"
    !insertmacro FileCriticalNonfatal "weaselARM64.dll"
    !insertmacro FileCriticalNonfatal "weaselARM64X.dll"
  ${EndIf}
  ; install x64 build for NativeARM64_WINDOWS11 and NativeAMD64_WINDOWS11
  ${If} ${AtLeastWin11} ; Windows 11 and above
    ${If} ${IsNativeARM64}
      !insertmacro FileCritical "WeaselDeployer.exe"
      !insertmacro FileCritical "WeaselServer.exe"
      !insertmacro FileCritical "rime.dll"
      !insertmacro FileCritical "WinSparkle.dll"
    ${ElseIf} ${IsNativeAMD64}
      !insertmacro FileCritical "WeaselDeployer.exe"
      !insertmacro FileCritical "WeaselServer.exe"
      !insertmacro FileCritical "rime.dll"
      !insertmacro FileCritical "WinSparkle.dll"
    ${Else}
      !insertmacro FileCritical "Win32\WeaselDeployer.exe"
      !insertmacro FileCritical "Win32\WeaselServer.exe"
      !insertmacro FileCritical "Win32\rime.dll"
      !insertmacro FileCritical "Win32\WinSparkle.dll"
    ${Endif}
  ; install x64 build for NativeAMD64_BELLOW_WINDOWS11
  ${Else} ; Windows 10 or bellow
    ${If} ${IsNativeAMD64}
      !insertmacro FileCritical "WeaselDeployer.exe"
      !insertmacro FileCritical "WeaselServer.exe"
      !insertmacro FileCritical "rime.dll"
      !insertmacro FileCritical "WinSparkle.dll"
    ${Else}
      !insertmacro FileCritical "Win32\WeaselDeployer.exe"
      !insertmacro FileCritical "Win32\WeaselServer.exe"
      !insertmacro FileCritical "Win32\rime.dll"
      !insertmacro FileCritical "Win32\WinSparkle.dll"
    ${Endif}
  ${EndIf}
  SetOverwrite try

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

  ; Assert every critical binary was really written before the registry below
  ; claims the new version; failures are collected during the File block above.
  Call VerifyCriticalBinaries

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

  ; WeaselSetup registers the TSF server and copies the IME DLLs into
  ; System32 / SysWOW64 — those are the copies host processes actually load.
  ; Its copy_file() renames a locked target aside and retries, so they do get
  ; refreshed even while loaded, but a hard failure would leave stale IME
  ; DLLs behind. Capture the exit code instead of discarding it.
  ExecWait '"$INSTDIR\WeaselSetup.exe" $R2' $R4
  ${If} $R4 != 0
    IfSilent setup_failed_silent
    MessageBox MB_OK|MB_ICONSTOP "$(SETUPFAILED)"
    setup_failed_silent:
    Abort "$(SETUPFAILED)"
  ${EndIf}

  ; Verify the system-wide IME copies match the freshly installed ones; a
  ; stale System32/SysWOW64 weasel.dll is a half-install that the $INSTDIR
  ; probe above cannot see.
  Call VerifySystemImeDlls

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
  Push $R1
  StrCpy $R1 "$INSTDIR\WeaselServer.exe"
  Call un.StopWeaselServer
  Pop $R1

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
