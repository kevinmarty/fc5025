[Setup]
OutputBaseFilename=FC5025_Windows_Setup
OutputDir=.
OutputManifestFile=manifest.txt
AllowCancelDuringInstall=no
AlwaysShowComponentsList=no
; allow installing on x86 (with libusb 0.1.12.2, win98se and newer)
; and amd64 (with libusb 1.2.5.0, winxp and newer).
; ia64 should also be possible if we had a machine to test on.
ArchitecturesAllowed=x86 x64
ArchitecturesInstallIn64BitMode=x64 ia64
AppName = USB 5.25" Floppy Drivers
AppVerName = USB 5.25" Floppy Drivers
DefaultDirName = {pf32}\FC5025
DefaultGroupName = USB Floppy
SolidCompression = yes
; gtk 2.6.10 runs on win98, me, nt4
; libusb-win32 runs on win98se, winme, win2k, winxp
; so, require at least win98se
MinVersion=4.1.2222,4.0

[Messages]
; default FinishedLabel says "may be launched by selecting the installed icons."
; say something more accurate instead.
FinishedLabel=Setup has finished installing [name] on your computer. You can start the Disk Image and Browse tool using the Start menu.

[Files]
; WinDIB executable
Source: "windib.exe"; DestDir: "{app}"
; Command line tools
Source: "..\cmd\OBJ.win32\fcformats.exe"; DestDir: "{app}"
Source: "..\cmd\OBJ.win32\fcdrives.exe"; DestDir: "{app}"
Source: "..\cmd\OBJ.win32\fcimage.exe"; DestDir: "{app}"
Source: "..\cmd\OBJ.win32\fcbrowse.exe"; DestDir: "{app}"
Source: "cmd.pdf"; DestDir: "{app}"
; GTK libraries
Source: "..\gtk_installer\gtk_install_files\bin\*"; DestDir: "{app}"
Source: "..\gtk_installer\gtk_install_files\etc\*"; DestDir: "{app}\etc"; Flags: recursesubdirs createallsubdirs
Source: "..\gtk_installer\gtk_install_files\lib\*"; DestDir: "{app}\lib"; Flags: recursesubdirs createallsubdirs
Source: "..\gtk_installer\gtk_install_files\share\*"; DestDir: "{app}\share"; Flags: recursesubdirs createallsubdirs
; libusb-win32 libraries
Source: "..\libusb-win32-device-bin-0.1.12.2\bin\libusb0.dll"; DestDir: "{app}"
Source: "..\libusb-win32-device-bin-0.1.12.2\bin\libusb0.dll"; DestDir: "{sys}"; Flags: replacesameversion restartreplace uninsneveruninstall sharedfile; Check: not Is64BitInstallMode
Source: "..\libusb-win32-bin-1.2.5.0\bin\amd64\libusb0.dll"; DestDir: "{app}"; DestName: "libusb0_amd64.dll"; Check: Is64BitInstallMode
;Source: "..\libusb-win32-bin-1.2.5.0\bin\ia64\libusb0.dll"; DestDir: "{app}"; DestName: "libusb0_ia64.dll"; Check: itanium
Source: "..\libusb-win32-bin-1.2.5.0\bin\x86\libusb0_x86.dll"; DestDir: "{app}"; Check: Is64BitInstallMode
; libusb-win32 drivers for rundll32 installation - for win98se thru win 7
Source: "..\libusb-win32-device-bin-0.1.12.2\bin\libusb0.sys"; DestDir: "{app}"; Check: not Is64BitInstallMode; OnlyBelowVersion: 0,6.02
Source: "..\libusb-win32-bin-1.2.5.0\bin\amd64\libusb0.sys"; DestDir: "{app}"; Check: Is64BitInstallMode; OnlyBelowVersion: 0,6.02
;Source: "..\libusb-win32-bin-1.2.5.0\bin\ia64\libusb0.sys"; DestDir: "{app}"; Check: itanium
Source: "fc5025.inf"; DestDir: "{app}"; Check: not Is64BitInstallMode; OnlyBelowVersion: 0,6.02
Source: "fc5025-win64.inf"; DestDir: "{app}"; Check: Is64BitInstallMode; OnlyBelowVersion: 0,6.02
; libusb-win32 driver installer - for win 8
Source: "..\wdi-simple-1.2.2\wdi32.exe"; DestDir: "{tmp}"; Check: not Is64BitInstallMode; MinVersion: 0,6.02
Source: "..\wdi-simple-1.2.2\wdi64.exe"; DestDir: "{tmp}"; Check: Is64BitInstallMode; MinVersion: 0,6.02

[Icons]
Name: "{group}\Disk Image and Browse"; Filename: "{app}\windib.exe"
Name: "{group}\Uninstall"; Filename: "{uninstallexe}"

[Run]
; invoke libusb's DLL to install the .inf file (for Win98SE thru Windows 7)
Filename: "rundll32"; Parameters: "libusb0.dll,usb_install_driver_np_rundll {app}\fc5025.inf"; StatusMsg: "Installing driver (this may take a few seconds) ..."; Check: not Is64BitInstallMode; OnlyBelowVersion: 0,6.02
Filename: "rundll32"; Parameters: """{app}\libusb0_amd64.dll"",usb_install_driver_np_rundll {app}\fc5025-win64.inf"; StatusMsg: "Installing driver (this may take a few seconds) ..."; Check: Is64BitInstallMode; OnlyBelowVersion: 0,6.02
;Filename: "rundll32"; Parameters: "{app}\libusb0_ia64.dll,usb_install_driver_np_rundll {app}\fc5025-win64.inf"; StatusMsg: "Installing driver (this may take a few seconds) ..."; Check: itanium
; use libwdi to install the libusb drivers (for Windows 8)
Filename: "{tmp}\wdi32.exe"; Flags: "runhidden"; Parameters: " --name ""FC5025 Floppy Controller"" --manufacturer ""Device Side Data"" --vid 0x16c0 --pid 0x06d6 --type 1 --progressbar={wizardhwnd}"; StatusMsg: "Installing driver (this may take a few seconds) ..."; MinVersion: 0,6.02; Check: not Is64BitInstallMode
Filename: "{tmp}\wdi64.exe"; Flags: "runhidden"; Parameters: " --name ""FC5025 Floppy Controller"" --manufacturer ""Device Side Data"" --vid 0x16c0 --pid 0x06d6 --type 1 --progressbar={wizardhwnd}"; StatusMsg: "Installing driver (this may take a few seconds) ..."; MinVersion: 0,6.02; Check: Is64BitInstallMode

