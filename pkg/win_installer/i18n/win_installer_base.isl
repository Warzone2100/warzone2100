; SPDX-License-Identifier: MIT
;
; *** Warzone 2100 Installer extra messages ***
;

[CustomMessages]

; *** Components:

CoreGame=Core Game
Addons=Addons
HQTerrain=High Quality Terrain
AddonMusic=Addon Music
AddonCampaigns=Addon Campaigns
Videos=Videos
EnglishHQ=English (High Quality)
EnglishStd=English (Standard)
EnglishLQ=English (Low Quality)
KeepExisting=Keep Existing
MSSysLibraries=Important Microsoft Runtime DLLs

; *** Advanced Options

AdvancedButton=&Advanced

; *** wzadvancedoptionsform.iss:

AdvancedOptionsTitle=Advanced Options - %1
StandardInstall=Standard Install
StandardInstallDesc=Install %1 on your PC. (Recommended)
StandardUpdate=Standard Install (Update)
StandardUpdateDesc=Update the existing install of %1 on your PC. (Recommended)
SideBySideInstall=Side-by-Side Install
SideBySideInstallDesc=Install this version of the game alongside any other versions, including the Standard Install. Shares settings and saves.
PortableInstall=Portable Install
PortableInstallDesc=Install to a USB drive, folder, etc. Fully self-contained in a single directory, including settings and saves.
InstallArchitecture=Install Architecture:

; *** Various Failure Messages:

UnableToDetermineArchPrompt=Unable to determine compatible architecture. Warzone 2100 may not support your system. Default to %1 anyway?
VideoDownloadErrorTitle=Error Downloading Game Videos
VideoDownloadErrorRetryPrompt=An error was encountered attempting to download the game videos. Try again?
VideoDownloadFailedTitle=Failed to Download Game Videos
VideoDownloadFailedText=Install will proceed anyway, but you may want to manually install the campaign video sequences later.

; *** vcredist.iss:

VCRuntimeInstalling=Installing Visual C++ Runtime Redistributable
VCRuntimeWaiting=Waiting for installation to complete...
VCRuntimeVerifying=Verifying Download
