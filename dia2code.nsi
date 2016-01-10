; dia2code
; Win32 Installer Source Code for NSIS 1.44

; (c) 2001 Steffen Macke <sdteffen@yahoo.com>

; ***************************************************************************
; *                                                                         *
; *   This program is free software; you can redistribute it and/or modify  *
; *   it under the terms of the GNU General Public License as published by  *
; *   the Free Software Foundation; either version 2 of the License, or     *
; *   (at your option) any later version.                                   *
; *                                                                         *
; ***************************************************************************


Name "dia2code 0.7"
LicenseText "Please read and agree to this license before continuiung."
LicenseData COPYING
ComponentText "This will install dia2code 0.7 on your system. Select which options you want set up."
DirText "Select a directory to install the program in."
UninstallText "This will uninstall the dia2code 0.7. Hit Next to uninstall, or Cancel to cancel."
UninstallExeName uninstall-dia2code-0.7.exe
OutFile dia2code-0.7-setup.exe
InstallDir $PROGRAMFILES\dia2code
Icon dia.ico
EnabledBitmap dia1.bmp
DisabledBitmap dia2.bmp
InstType Typical

InstallDirRegKey HKEY_LOCAL_MACHINE "Software\dia2code" "instpath"
SetOverwrite on

Section "dia2code 0.7"
SetOutPath $INSTDIR
File COPYING
File README
File README.win32
SetOutPath $INSTDIR\bin
File dia2code\dia2code.exe
File dia2code\libxml2.dll

WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\dia2code" "DisplayName" "dia2code 0.7 (remove only)"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\dia2code" "UninstallString" '"$INSTDIR\uninstall-dia2code-0.7.exe"'






WriteRegStr HKEY_CLASSES_ROOT "diaFile\Shell\DefaultIcon" "" "$INSTDIR\bin\dia.exe"

WriteRegStr HKEY_LOCAL_MACHINE "Software\dia2code" "instpath" $INSTDIR
SectionEnd

Section "Context Menu Entries"
SectionIn 1
WriteRegStr HKEY_CLASSES_ROOT ".dia" "" "diaFile"
WriteRegStr HKEY_CLASSES_ROOT ".dia" "Content Type" "application/dia"
WriteRegStr HKEY_CLASSES_ROOT "diaFile" "" "diaFile"
WriteRegBin HKEY_CLASSES_ROOT "diaFile" "EditFlags" 00000100
WriteRegStr HKEY_CLASSES_ROOT "diaFile\Shell\createsql" "" "Create SQL definition"
WriteRegStr HKEY_CLASSES_ROOT "diaFile\Shell\createsql\command" "" '"$INSTDIR\bin\dia2code.exe" -t sql "%1"'
WriteRegStr HKEY_CLASSES_ROOT "diaFile\Shell\createc" "" "Create C code"
WriteRegStr HKEY_CLASSES_ROOT "diaFile\Shell\createc\command" "" '"$INSTDIR\bin\dia2code.exe" -t c "%1"'
WriteRegStr HKEY_CLASSES_ROOT "diaFile\Shell\createcpp" "" "Create C++ code"
WriteRegStr HKEY_CLASSES_ROOT "diaFile\Shell\createcpp\command" "" '"$INSTDIR\bin\dia2code.exe" -t cpp "%1"'
WriteRegStr HKEY_CLASSES_ROOT "diaFile\Shell\createjava" "" "Create Java code"
WriteRegStr HKEY_CLASSES_ROOT "diaFile\Shell\createjava\command" "" '"$INSTDIR\bin\dia2code.exe" -t java "%1"'
WriteRegStr HKEY_CLASSES_ROOT "diaFile\Shell\createada" "" "Create ADA code"
WriteRegStr HKEY_CLASSES_ROOT "diaFile\Shell\createada\command" "" '"$INSTDIR\bin\dia2code.exe" -t ada "%1"'
WriteRegStr HKEY_CLASSES_ROOT "diaFile\Shell\createpython" "" "Create Python code"
WriteRegStr HKEY_CLASSES_ROOT "diaFile\Shell\createpython\command" "" '"$INSTDIR\bin\dia2code.exe" -t python "%1"'
SectionEnd

Section "Start Menu Entries"
SectionIn 1
SetOutPath "$SMPROGRAMS\dia2code"
File "dia2code Homepage.url"
File "Report a Bug.url"
File /oname=README.txt README
File /oname=README.win32.txt README.win32
SetOutPath $INSTDIR\bin
CreateShortCut "$SMPROGRAMS\dia2code\Uninstall dia2code 0.7.lnk" "$INSTDIR\uninstall-dia2code-0.7.exe" ""  "" 0

SectionEnd

Section Uninstall
Delete $INSTDIR\bin\dia2code.exe
Delete $INSTDIR\bin\libxml2.dll
RMDir  $INSTDIR\bin
Delete $INSTDIR\COPYING
Delete $INSTDIR\README
Delete $INSTDIR\README.win32
Delete $INSTDIR\uninstall-dia2code-0.7.exe
RMDir  $INSTDIR
Delete "$SMPROGRAMS\dia2code\*.*"
RMDir "$SMPROGRAMS\dia2code"

DeleteRegValue HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\dia2code" "UninstallString"
DeleteRegValue HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\dia2code" "DisplayName"
DeleteRegKey HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\dia2code"
SectionEnd
