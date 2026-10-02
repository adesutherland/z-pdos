; SPDX-License-Identifier: MIT
; Adapted from the cREXX-RAG per-user installer interface.
; Classic-tools per-user installer. Does not set CREXX_HOME or touch CREXX's install.
Unicode true
RequestExecutionLevel user
SetCompressor /SOLID lzma
ManifestDPIAware true
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"
!include "WinMessages.nsh"

!ifdef ZPDOS_SIGN_HELPER
  !system '"${ZPDOS_SIGN_HELPER}" --nsis-plugins "${NSISDIR}/Plugins/x86-unicode" "${ZPDOS_UNINSTALL_FILES}.plugins"' = 0
  !addplugindir /x86-unicode "${ZPDOS_UNINSTALL_FILES}.plugins"
  !uninstfinalize '"${ZPDOS_SIGN_HELPER}" "%1"' = 0
!endif

Name "z/PDOS Classic Tools ${ZPDOS_VERSION}"
OutFile "${ZPDOS_OUTPUT}"
InstallDir "$LOCALAPPDATA\Programs\z-pdos"
InstallDirRegKey HKCU "Software\z-pdos" "InstallDir"
VIProductVersion "0.1.0.0"
VIAddVersionKey "ProductName" "z/PDOS Classic Tools"
VIAddVersionKey "FileDescription" "z/PDOS Classic Tools Windows x64 Installer"
VIAddVersionKey "FileVersion" "${ZPDOS_VERSION}"
VIAddVersionKey "LegalCopyright" "z/PDOS Classic Tools contributors"
!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_ICONSTOP "z/PDOS Classic Tools requires 64-bit Windows."
    Abort
  ${EndIf}
  SetRegView 64
FunctionEnd

Function un.onInit
  SetRegView 64
FunctionEnd

; Registry API preserves long PATH values and their original representation.
; The directory is environment data, never PowerShell source interpolation.
!macro UpdatePath ACTION
  System::Call 'kernel32::SetEnvironmentVariable(t "ZPDOS_INSTALL_BIN", t "$INSTDIR\bin")'
  System::Call 'kernel32::SetEnvironmentVariable(t "ZPDOS_PATH_ACTION", t "${ACTION}")'
  nsExec::ExecToStack `"$WINDIR\Sysnative\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "$INSTDIR\share\z-pdos\update-user-path.ps1"`
  Pop $0
  Pop $1
  System::Call 'kernel32::SetEnvironmentVariable(t "ZPDOS_INSTALL_BIN", p 0)'
  System::Call 'kernel32::SetEnvironmentVariable(t "ZPDOS_PATH_ACTION", p 0)'
  ${If} $0 != 0
    DetailPrint "PATH update failed: $1"
    SetErrorLevel 1
    Abort
  ${EndIf}
  SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000
!macroend

Section "z/PDOS Classic Tools"
  SetOutPath "$INSTDIR"
  File /r "${ZPDOS_PAYLOAD}\*"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKCU "Software\z-pdos" "InstallDir" "$INSTDIR"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\z-pdos" "DisplayName" "z/PDOS Classic Tools ${ZPDOS_VERSION}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\z-pdos" "DisplayVersion" "${ZPDOS_VERSION}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\z-pdos" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\z-pdos" "QuietUninstallString" '"$INSTDIR\Uninstall.exe" /S'
  !insertmacro UpdatePath add
SectionEnd

Section "Uninstall"
  !insertmacro UpdatePath remove
  !include "${ZPDOS_UNINSTALL_FILES}"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"
  DeleteRegKey HKCU "Software\z-pdos"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\z-pdos"
SectionEnd
