Name "Bomberclone Installer"

OutFile "bomberclone_install.exe"

; The default installation directory
InstallDir $PROGRAMFILES\bomberclone

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\bomberclone" "Install_Dir"

LicenseText "License"
LicenseData "COPYING"

;--------------------------------
; Pages

Page license
Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "Bomberclone (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File "src/bomberclone.exe"
  File "COPYING"
  File "README"
  File "AUTHORS"
  File "ChangeLog"
  File "lib/jpeg.dll"
  File "lib/libpng1.dll"
  File "lib/SDL.dll"
  File "lib/SDL_image.dll"
  File "lib/SDL_mixer.dll"
  File "lib/zlib.dll"
  SetOutPath "$INSTDIR\data"
  File /r "data/"
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\bomberclone "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Bomberclone" "DisplayName" "Bomberclone"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Bomberclone" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Bomberclone" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Bomberclone" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\BomberClone"
  CreateShortCut "$SMPROGRAMS\BomberClone\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\BomberClone\BomberClone.lnk" "$INSTDIR\bomberclone.exe" "" "$INSTDIR\bomberclone.exe" 0
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Bomberclone"
  DeleteRegKey HKLM SOFTWARE\bomberclone

  ; Remove files and uninstaller
  Delete $INSTDIR\bomberclone.exe
  Delete $INSTDIR\AUTHORS
  Delete $INSTDIR\ChangeLog
  Delete $INSTDIR\COPYING
  Delete $INSTDIR\README
  Delete $INSTDIR\uninstall.exe
  Delete $INSTDIR\jpeg.dll
  Delete $INSTDIR\libpng1.dll
  Delete $INSTDIR\SDL.dll
  Delete $INSTDIR\SDL_image.dll
  Delete $INSTDIR\SDL_mixer.dll
  Delete $INSTDIR\zlib.dll
  
  RMDir /r $INSTDIR\data
  
  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\bomberclone\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\bomberclone"
  RMDir /r "$INSTDIR"
  Delete /REBOOTOK "$INSTDIR"

SectionEnd
