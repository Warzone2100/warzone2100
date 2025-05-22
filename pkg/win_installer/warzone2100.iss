// SPDX-License-Identifier: MIT
//
// Warzone 2100 Inno Setup Script
// Copyright (c) 2025 Warzone 2100 Project
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#if ComparePackedVersion(Ver, EncodeVer(6,4,3,0)) < 0
#error This script requires Inno Setup 6.4.3+ to compile
#endif

#define private

// Points to the source dir in which the staged files to be packaged are located
#ifndef SOURCE_DIR 
#define SOURCE_DIR "files"
#endif

#define WZ_BINDIR "bin"
#ifndef WZ_DATADIR
#define WZ_DATADIR "data"
#endif
#define WZ_DOCDIR "doc"
#define WZ_LOCALEDIR "locale"

#ifndef WZ_EXECUTABLE_NAME
#define WZ_EXECUTABLE_NAME "warzone2100.exe"
#endif

#ifndef WZ_SETUP_ICON
#define WZ_SETUP_ICON AddBackslash(SourcePath) + "resources\wzsetupicon.ico"
#endif

#define WZ_PORTABLEMODE_CONFIG_FILE_NAME ".portable"

// Check for architectures that are available in the SOURCE_DIR staging area
#define WZ_ARCHITECTURE_X86 0
#define WZ_ARCHITECTURE_X64 0
#define WZ_ARCHITECTURE_ARM64 0
#if FileExists(AddBackslash(SOURCE_DIR) + AddBackslash(WZ_BINDIR) + "x86\" + WZ_EXECUTABLE_NAME) != 0
  #define WZ_ARCHITECTURE_X86 1
  #pragma message "Found bin\x86\"
#endif
#if FileExists(AddBackslash(SOURCE_DIR) + AddBackslash(WZ_BINDIR) + "x64\" + WZ_EXECUTABLE_NAME) != 0
  #define WZ_ARCHITECTURE_X64 1
  #pragma message "Found bin\x64\"
#endif
#if FileExists(AddBackslash(SOURCE_DIR) + AddBackslash(WZ_BINDIR) + "arm64\" + WZ_EXECUTABLE_NAME) != 0
  #define WZ_ARCHITECTURE_ARM64 1
  #pragma message "Found bin\arm64\"
#endif
#if WZ_ARCHITECTURE_X86 == 0 && WZ_ARCHITECTURE_X64 == 0 && WZ_ARCHITECTURE_ARM64 == 0
  #pragma error "Did not find any build architectures in SOURCE_DIR\bin\: " + AddBackslash(SOURCE_DIR) + "bin\"
#endif

// AutoVersion: Extract product and file version info from the warzone2100.exe version info
#define WZ_EXTRACT_VERSION_BINARY_PATH AddBackslash(SOURCE_DIR) + AddBackslash(WZ_BINDIR) + "x64\" + WZ_EXECUTABLE_NAME
#if FileExists(WZ_EXTRACT_VERSION_BINARY_PATH) == 0
#define WZ_EXTRACT_VERSION_BINARY_PATH AddBackslash(SOURCE_DIR) + AddBackslash(WZ_BINDIR) + "arm64\" + WZ_EXECUTABLE_NAME
#endif
#if FileExists(WZ_EXTRACT_VERSION_BINARY_PATH) == 0
#define WZ_EXTRACT_VERSION_BINARY_PATH AddBackslash(SOURCE_DIR) + AddBackslash(WZ_BINDIR) + "x86\" + WZ_EXECUTABLE_NAME
#endif
#if FileExists(WZ_EXTRACT_VERSION_BINARY_PATH) == 0
  #error Can't find warzone2100 executable to check for version info - please ensure SOURCE_DIR is defined properly
#endif

#pragma message "*******************************************************"
#pragma message "[AUTOVERSION] Extracting version information from: " + WZ_EXTRACT_VERSION_BINARY_PATH

#include "wzautorevision.iss"

#define ExtractedProductVersion GetFileProductVersion(WZ_EXTRACT_VERSION_BINARY_PATH)
#if ExtractedProductVersion == ""
  #error Failed to get ProductVersion from warzone2100 executable
#endif
#pragma message "[AUTOVERSION] ProductVersion: " + ExtractedProductVersion

#define MyAppInstallerFileVersion GetVersionNumbersString(WZ_EXTRACT_VERSION_BINARY_PATH)
#if MyAppInstallerFileVersion == ""
  #error Failed to get FileVersion from warzone2100 executable
#endif
#pragma message "[AUTOVERSION] FileVersion: " + MyAppInstallerFileVersion

#define ExtractedVersionInfoComments GetStringFileInfo(WZ_EXTRACT_VERSION_BINARY_PATH, "Comments")
#if ExtractedVersionInfoComments == ""
  #error Failed to get Comments (and autorevision info) from warzone2100 executable
#endif

#define VCS_TAG GetAutoRevision_Tag(ExtractedVersionInfoComments)
#define VCS_BRANCH GetAutoRevision_Branch(ExtractedVersionInfoComments)
#define VCS_COMMIT GetAutoRevision_Commit(ExtractedVersionInfoComments)
#define VCS_EXTRA GetAutoRevision_Extra(ExtractedVersionInfoComments)

#pragma message "Tag: " + VCS_TAG
#pragma message "Branch: " + VCS_BRANCH
#pragma message "Commit: " + VCS_COMMIT
#pragma message "Extra: " + VCS_EXTRA

#if VCS_TAG == "" && VCS_BRANCH == "" && VCS_EXTRA == ""
  #error Revision info was unexpectedly empty
#endif

#if !(VCS_TAG == "")
  // On a tag (release build)
  #define MyBaseAppId "Warzone2100"
  #define MyAppName "Warzone 2100"
  #define MyAppVersion ExtractedProductVersion
  #define MyAppInstallerProductTextVersion MyAppVersion
#elif !(VCS_BRANCH == "")
  // On a branch:
  // - Use branch-specific base appid (truncating branch name if too long)
  //   - (Max Inno Setup AppId length: 127) - ("Warzone2100_b_" Length: 14) - (Short Hash Length: 7) = 106
  //   - This also matches the behavior of WZ's version_getVersionedAppDirFolderName(), which treats all non-tagged builds on a branch as "the same app" (and shares saves / settings amongst them)
  #define MyBaseAppId "Warzone2100_b_" + Copy(VCS_BRANCH, 1, 106)
  // - Use AppName with branch included (fit to something reasonable for display)
  #define MyAppName "Warzone 2100 [" + TruncateStrWithEllipsis(VCS_BRANCH, 28) + "]"
  // - Use a shortened version of the commit as the display version
  #define MyAppVersion ShortCommitHashFromCommit(VCS_COMMIT)
  // - Setup file product info text should include both the branch and the commit
  #define MyAppInstallerProductTextVersion VCS_BRANCH + " " + VCS_COMMIT
#else
  // Some other build (possibly a PR, etc) - just use the VCS_EXTRA details
  // NOTE: Since this uses a unique AppId for each build, the Standard install mode effectively acts as the Side-by-Side install mode
  // (and neither will share saves / settings with other builds)
  #define MyBaseAppId "Warzone2100_o_" + Copy(VCS_EXTRA, 1, 106)
  #define MyAppName "Warzone 2100 [" + VCS_EXTRA + "]"
  #define MyAppVersion ShortCommitHashFromCommit(VCS_COMMIT)
  #define MyAppInstallerProductTextVersion VCS_EXTRA
#endif

#pragma message "MyBaseAppId: '" + MyBaseAppId + "'"
#pragma message "MyAppName: '" + MyAppName + "'"
#pragma message "MyAppVersion: '" + MyAppVersion + "'"
#pragma message "MyAppInstallerProductTextVersion: '" + MyAppInstallerProductTextVersion + "'"

#pragma message "*******************************************************"

// Sanity checks
#if DirExists(AddBackslash(SOURCE_DIR) + WZ_DATADIR) == 0
#pragma error "Setup script expects data folder to be '" WZ_DATADIR "' - if WZ_DATADIR define does not match what the built warzone2100 executable expects, the installer won't install to the proper location!"
#endif
#if DirExists(AddBackslash(SOURCE_DIR) + WZ_DOCDIR) == 0
#pragma error "Setup script expects docs folder to be '" WZ_DOCDIR "', but it seems to be missing at: " + AddBackslash(SOURCE_DIR) + WZ_DOCDIR
#endif
#if DirExists(AddBackslash(SOURCE_DIR) + WZ_LOCALEDIR) == 0
#pragma error "Setup script expects locale folder to be '" WZ_LOCALEDIR "', but it seems to be missing at: " + AddBackslash(SOURCE_DIR) + WZ_LOCALEDIR
#endif

#define MyAppPublisher "Warzone 2100 Project"
#define MyAppPublisherURL "https://github.com/Warzone2100"
#define MyAppURL "https://wz2100.net"
#define MyAppExeName WZ_EXECUTABLE_NAME

#define MSVCRUNTIME
#define UNINSTALL_DATA_SUBFOLDER "uninst"

#define MyArchitecturesAllowed ""
#if WZ_ARCHITECTURE_X86
  #define MyArchitecturesAllowed MyArchitecturesAllowed + " x86compatible"
#endif
#if WZ_ARCHITECTURE_X64
  #define MyArchitecturesAllowed MyArchitecturesAllowed + " x64compatible"
#endif
#if WZ_ARCHITECTURE_ARM64
  #define MyArchitecturesAllowed MyArchitecturesAllowed + " arm64"
#endif
#pragma message "ArchitecturesAllowed=" + MyArchitecturesAllowed

// Includes

#pragma include __INCLUDE__ + ";" + AddBackslash(SourcePath) + "innohelperscripts"

#define public TARGETARCH_SUPPORTS_X86 WZ_ARCHITECTURE_X86
#define public TARGETARCH_SUPPORTS_X64 WZ_ARCHITECTURE_X64
#define public TARGETARCH_SUPPORTS_ARM64 WZ_ARCHITECTURE_ARM64
#include <targetinstallarch.iss>
#include <installmodes.iss>
#include <vcredist.iss>
#include <retryabledl.iss>
#define public ADVANCEDOPTIONSFORM_NO_DEFINE_CUSTOMMESSAGES
#include <forms\advancedoptionsform.iss>

// Setup Script

[Setup]
SourceDir={#SOURCE_DIR}

// Output Settings
OutputDir=output
OutputBaseFilename=warzone2100_win_installer
OutputManifestFile=setup-manifest.txt

// App Information
AppId={code:InstallModes_AppId|{#MyBaseAppId},{#MyAppVersion}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} ({#MyAppVersion})
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppPublisherURL}
AppSupportURL={#MyAppURL}/support/
AppUpdatesURL={#MyAppURL}

// File VersionInfo
VersionInfoCompany={#MyAppPublisher}
VersionInfoCopyright=Copyright © 2024-2025 Warzone 2100 Project (https://github.com/Warzone2100)
VersionInfoDescription={#MyAppName} Installer
VersionInfoProductName={#MyAppName}
VersionInfoProductTextVersion={#MyAppInstallerProductTextVersion}
VersionInfoVersion={#MyAppInstallerFileVersion}
SetupIconFile={#WZ_SETUP_ICON}

// Specifies that setup can run on basically any Windows (including x86-compatible, x64-compatible, and arm64)
ArchitecturesAllowed={#MyArchitecturesAllowed}
// Should install in 64-bit mode on any 64-bit Windows (regardless of OS architecture)
ArchitecturesInstallIn64BitMode=win64

// Default setting is to install for all users (which requires admin)
PrivilegesRequired=admin
// But show a dialog to allow the user to choose
PrivilegesRequiredOverridesAllowed=commandline dialog

// Unfortunately, these are both required to be 'no' when setting the AppId using constants
UsePreviousPrivileges=no
UsePreviousLanguage=no

// Configure the default dir name based on the install mode
DefaultDirName={code:InstallModes_DefaultDirName|{#MyAppName},{#MyAppVersion}}

// Compression Settings
Compression=lzma2/ultra64
SolidCompression=yes
LZMAUseSeparateProcess=yes
LZMANumFastBytes=273
LZMADictionarySize=524288

// Uninstall settings
Uninstallable=not IsPortableMode
UninstallDisplayName={code:InstallModes_AppInstallationName|{#MyAppName},{#MyAppVersion}}
UninstallFilesDir={app}\{#UNINSTALL_DATA_SUBFOLDER}
UninstallDisplayIcon={app}\{#WZ_BINDIR}\{#MyAppExeName}

// Installer Style
WizardStyle=modern
WizardSizePercent=110
DefaultDialogFontName=Segoe UI
AlwaysShowComponentsList=yes
AlwaysShowDirOnReadyPage=yes
WizardImageFile={#AddBackslash(SourcePath) + "resources\wzwizardimage.bmp"}

// Install Pages
// - Show the welcome page - it's where we provide the "Advanced" button
DisableWelcomePage=no
// - Never show the program group page
DisableProgramGroupPage=yes
// - Rely on custom logic in ShouldSkipPage event handler for the DirPage (so set DisableDirPage=no)
DisableDirPage=no

// Display licensing and source code info & links before install
InfoBeforeFile={#SourcePath}\wz2100_innowizard_infobefore.rtf

[Languages]
// Base language is en
Name: "en"; MessagesFile: "compiler:Default.isl,{#SourcePath}\i18n\win_installer_base.isl"
// Additional languages with "official" base Inno Setup translations
Name: "ar"; MessagesFile: "compiler:Languages\Arabic.isl"
Name: "bg"; MessagesFile: "compiler:Languages\Bulgarian.isl"
Name: "ca"; MessagesFile: "compiler:Languages\Catalan.isl"
Name: "co"; MessagesFile: "compiler:Languages\Corsican.isl"
Name: "cs"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "da"; MessagesFile: "compiler:Languages\Danish.isl"
Name: "de"; MessagesFile: "compiler:Languages\German.isl"
Name: "es"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "fi"; MessagesFile: "compiler:Languages\Finnish.isl"
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"
Name: "he"; MessagesFile: "compiler:Languages\Hebrew.isl"
Name: "hu"; MessagesFile: "compiler:Languages\Hungarian.isl"
Name: "hy"; MessagesFile: "compiler:Languages\Armenian.isl"
Name: "is"; MessagesFile: "compiler:Languages\Icelandic.isl"
Name: "it"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "ja"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "ko"; MessagesFile: "compiler:Languages\Korean.isl"
Name: "nb"; MessagesFile: "compiler:Languages\Norwegian.isl"
Name: "nl"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "pl"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "pt_PT"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "pt_BR"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "sk"; MessagesFile: "compiler:Languages\Slovak.isl"
Name: "sl"; MessagesFile: "compiler:Languages\Slovenian.isl"
Name: "sv"; MessagesFile: "compiler:Languages\Swedish.isl"
Name: "ta"; MessagesFile: "compiler:Languages\Tamil.isl"
Name: "tr"; MessagesFile: "compiler:Languages\Turkish.isl"
Name: "uk"; MessagesFile: "compiler:Languages\Ukrainian.isl"

[LangOptions]
// Future TODO: 
// - Ideally this should not override languages that specify a larger size (the default is 8),
//   although there does seem to be a way to easily do that. Do any used language .isl files have greater?
//   Would be nice if there was a "MinimumDialogFontSize" option.
DialogFontSize=9

[Messages]
AboutSetupNote=Additional scripts:%nhttps://github.com/past-due/innohelperscripts

// [Types]
// No need to define Types, because we use the built-in [full, compact, custom]

[Components]
Name: "core"; Description: "{cm:CoreGame}"; Types: full compact custom; Flags: fixed
Name: "addons"; Description: "{cm:Addons}"; Types: full compact custom; Flags: disablenouninstallwarning
Name: "addons\terrain_hq"; Description: "{cm:HQTerrain}"; Types: full compact custom; Flags: disablenouninstallwarning
Name: "addons\campaigns"; Description: "{cm:AddonCampaigns}"; Types: full compact custom; Flags: disablenouninstallwarning
Name: "addons\music"; Description: "{cm:AddonMusic}"; Types: full compact custom; Flags: disablenouninstallwarning
Name: "videos"; Description: "{cm:Videos}"; Types: full custom; Flags: disablenouninstallwarning
Name: "videos\english_hq"; Description: "{cm:EnglishHQ}"; Types: full custom; Flags: exclusive disablenouninstallwarning
Name: "videos\english_std"; Description: "{cm:EnglishStd}"; Types: full custom; Flags: exclusive disablenouninstallwarning
Name: "videos\english_lq"; Description: "{cm:EnglishLQ}"; Types: custom; Flags: exclusive disablenouninstallwarning
Name: "videos\keep_existing"; Description: "{cm:KeepExisting}"; Flags: exclusive fixed disablenouninstallwarning
#ifdef MSVCRUNTIME
Name: "vcruntime"; Description: "{cm:MSSysLibraries}"; Types: full compact custom; ExtraDiskSpaceRequired: 1048576; Flags: disablenouninstallwarning; Check: not IsWindows10OrLater()
#endif

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; Check: not IsPortableMode

[InstallDelete]
// New versions may change which files ship with the game, so explicitly clean up older data files pre-install to allow upgrade installs
Type: filesandordirs; Name: "{app}\{#WZ_DATADIR}\mods"
Type: filesandordirs; Name: "{app}\{#WZ_DATADIR}\music"
Type: filesandordirs; Name: "{app}\{#WZ_DATADIR}\terrain_overrides"
Type: files; Name: "{app}\{#WZ_DATADIR}\base.wz"
Type: files; Name: "{app}\{#WZ_DATADIR}\mp.wz"
// Do *not* delete "{app}\{#WZ_DATADIR}\sequences.wz" (user may have manually installed a copy, and may want to keep it when doing an over-the-top install / upgrade)
// Type: filesandordirs; Name: "{app}\{#WZ_DOCDIR}"

[Files]
// Core Binaries (only one architecture's set gets installed, based on the target install architecture)
#if WZ_ARCHITECTURE_X64
Source: "{#WZ_BINDIR}\x64\*"; DestDir: "{app}\{#WZ_BINDIR}"; Components: core; Flags: ignoreversion solidbreak; Check: IsTargettingInstallArchitecture('x64')
#endif
#if WZ_ARCHITECTURE_ARM64
Source: "{#WZ_BINDIR}\arm64\*"; DestDir: "{app}\{#WZ_BINDIR}"; Components: core; Flags: ignoreversion solidbreak; Check: IsTargettingInstallArchitecture('arm64')
#endif
#if WZ_ARCHITECTURE_X86
Source: "{#WZ_BINDIR}\x86\*"; DestDir: "{app}\{#WZ_BINDIR}"; Components: core; Flags: ignoreversion solidbreak; Check: IsTargettingInstallArchitecture('x86')
#endif
// Core Data: Game Data
Source: "{#WZ_DATADIR}\*"; Excludes: "\mods\*,\music\*,\terrain_overrides*"; DestDir: "{app}\{#WZ_DATADIR}"; Components: core; Flags: ignoreversion recursesubdirs createallsubdirs solidbreak
// Core Data: Language Files
Source: "{#WZ_LOCALEDIR}\*"; DestDir: "{app}\{#WZ_LOCALEDIR}"; Components: core; Flags: ignoreversion recursesubdirs createallsubdirs
// Core Data: Documentation
Source: "{#WZ_DOCDIR}\*"; DestDir: "{app}\{#WZ_DOCDIR}"; Components: core; Flags: ignoreversion recursesubdirs createallsubdirs
// Core Data: Classic Campaign Mod
Source: "{#WZ_DATADIR}\mods\campaign\wz2100_camclassic.wz"; DestDir: "{app}\{#WZ_DATADIR}\mods\campaign"; Components: core; Flags: ignoreversion
// Core Data: Classic Terrain Pack
Source: "{#WZ_DATADIR}\terrain_overrides\classic.wz"; DestDir: "{app}\{#WZ_DATADIR}\terrain_overrides"; Components: core; Flags: ignoreversion
// Core Data: Base Music
Source: "{#WZ_DATADIR}\music\*"; DestDir: "{app}\{#WZ_DATADIR}\music"; Components: core; Flags: ignoreversion
Source: "{#WZ_DATADIR}\music\albums\original_soundtrack\*"; DestDir: "{app}\{#WZ_DATADIR}\music\albums\original_soundtrack"; Components: core; Flags: ignoreversion recursesubdirs createallsubdirs
// Core Data: Portable Mode Files
Source: "{#AddBackslash(SourcePath) + "launch_warzone.bat"}"; DestDir: "{app}"; Components: core; Flags: ignoreversion; Check: IsPortableMode
// - Note: Creation of the .portable mode config file occurs via code in CurStepChanged(CurStep = ssPostInstall)
// Addon: High Quality Terrain
Source: "{#WZ_DATADIR}\terrain_overrides\high.wz"; DestDir: "{app}\{#WZ_DATADIR}\terrain_overrides"; Components: addons\terrain_hq; Flags: ignoreversion
// Addon Mods
Source: "{#WZ_DATADIR}\mods\*"; Excludes: "\campaign\wz2100_camclassic.wz"; DestDir: "{app}\{#WZ_DATADIR}\mods"; Components: addons\campaigns; Flags: ignoreversion recursesubdirs createallsubdirs
// Addon Music
Source: "{#WZ_DATADIR}\music\albums\*"; Excludes: "\original_soundtrack\*"; DestDir: "{app}\{#WZ_DATADIR}\music\albums"; Components: addons\music; Flags: ignoreversion recursesubdirs createallsubdirs
// Sequences (only one will be selected and downloaded on-demand)
Source: "{tmp}\sequences.wz"; DestDir: "{app}\{#WZ_DATADIR}"; DestName: "sequences.wz"; Components: videos/english_hq; ExternalSize: 964260204; Flags: external skipifsourcedoesntexist
Source: "{tmp}\sequences.wz"; DestDir: "{app}\{#WZ_DATADIR}"; DestName: "sequences.wz"; Components: videos/english_std; ExternalSize: 585663488; Flags: external skipifsourcedoesntexist
Source: "{tmp}\sequences.wz"; DestDir: "{app}\{#WZ_DATADIR}"; DestName: "sequences.wz"; Components: videos/english_lq; ExternalSize: 169657344; Flags: external skipifsourcedoesntexist

[Icons]
Name: "{autoprograms}\{code:InstallModes_AppInstallationName|{#MyAppName},{#MyAppVersion}}"; Filename: "{app}\{#WZ_BINDIR}\{#MyAppExeName}"; Check: not IsPortableMode and not WizardNoIcons
Name: "{autodesktop}\{code:InstallModes_AppInstallationName|{#MyAppName},{#MyAppVersion}}"; Filename: "{app}\{#WZ_BINDIR}\{#MyAppExeName}"; Tasks: desktopicon; Check: not IsPortableMode

[Run]
Filename: "{app}\{#WZ_BINDIR}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent runasoriginaluser

[UninstallDelete]
Type: files; Name: "{app}\{#WZ_BINDIR}\{#WZ_PORTABLEMODE_CONFIG_FILE_NAME}"

[Code]
var
  DownloadPage: TDownloadWizardPage;
  AdvancedOptionsButton: TNewButton;
  ComponentsPrepLastDirConfigured: String;
  LatestDownloadProgressUrl: String;
  LatestDownloadProgressStartTick: DWORD;
  LatestDownloadFailIfTooSlow: Boolean;

function GetTickCount(): DWORD;
external 'GetTickCount@kernel32.dll stdcall';

// Helpers

function IsWindows10OrLater(): Boolean;
begin
  Result := (GetWindowsVersion >= $0A000000);
end;

function OnDownloadProgress(const Url, FileName: String; const Progress, ProgressMax: Int64): Boolean;
var
  CurrTickCount: DWORD;
  ElapsedMilliseconds: DWORD;
begin
  if (Progress = 0) or (LatestDownloadProgressUrl <> Url) then begin
    LatestDownloadProgressUrl := Url;
    Log('Started new download: ' + Url);
    LatestDownloadProgressStartTick := GetTickCount();
    if CompareText(FileName, 'sequences.wz') = 0 then begin
      // Special handling for sequences downloads from SourceForge - check if download starts off too slow
      LatestDownloadFailIfTooSlow := WildcardMatch(Url, 'https://downloads.sourceforge.net/project/warzone2100/warzone2100/Videos/*');
    end else begin
      LatestDownloadFailIfTooSlow := False;
    end;
  end;
  if Progress = ProgressMax then begin
    Log(Format('Successfully downloaded file to "{tmp}": %s', [FileName]));
  end else if LatestDownloadFailIfTooSlow then begin
    // Handle failing slow downloads
    CurrTickCount := GetTickCount();
    ElapsedMilliseconds := CurrTickCount - LatestDownloadProgressStartTick;
    if (ElapsedMilliseconds >= 10000) and (Progress < 5000000) then begin
      // Fail this slow download - started off super slow (less than 5 MiB in the first 10 seconds)
      Log('Failing slow download: ' + Url);
      LatestDownloadProgressUrl := '';
      Result := False;
      Exit;
    end;
  end;
  Result := True;
end;

procedure AdvancedInstallOptionsButtonOnClick(Sender: TObject);
begin
  OpenAdvancedInstallOptionsModal('{#MyAppName}', '{#MyAppVersion}');
end;

function HasExistingKeepableSequences(): Boolean;
var
  destinationSequencesPath: String;
  sequencesFilesize : Int64;
begin
  // Check if the destination install directory already contains a non-empty sequences.wz
  destinationSequencesPath := AddBackslash(WizardDirValue()) + '{#WZ_DATADIR}\sequences.wz';
  if FileExists(destinationSequencesPath) and FileSize64(destinationSequencesPath, sequencesFilesize) and (sequencesFilesize > 0) then
  begin
    // Future TODO: If ever updating the video packages, check if the existing file matches an expected size / hash
    Result := True;
  end else begin
    Result := False;
  end;
end;

procedure PrepComponents;
var
  KeepExistingItemIndex: Integer;
begin
  // Only re-process if the dir has changed
  if WizardDirValue() = ComponentsPrepLastDirConfigured then Exit;
  ComponentsPrepLastDirConfigured := WizardDirValue();
  // Check if the ComponentsList has the "Keep Existing" component
  KeepExistingItemIndex := WizardForm.ComponentsList.Items.IndexOf(CustomMessage('KeepExisting'));
  if KeepExistingItemIndex < 0 then
  begin
    // No "Keep Existing" component is present?
    if Debugging then Log('No Keep Existing component is present?');
    Exit;
  end;
  // Check if the destination install directory already contains a non-empty sequences.wz
  if HasExistingKeepableSequences() then
  begin
    // Default to "Keep Existing"
    Log('An existing sequences.wz was found in the destination, defaulting to: Keep Existing');
    WizardSelectComponents('videos\keep_existing');
    WizardForm.ComponentsList.ItemEnabled[KeepExistingItemIndex] := True;
  end else begin
    if WizardIsComponentSelected('videos\keep_existing') then
      WizardSelectComponents('videos\english_std');
    WizardForm.ComponentsList.ItemEnabled[KeepExistingItemIndex] := False;
  end;
end;

function VideoDownloadWrapper(DownloadPage: TDownloadWizardPage; URLs: TStringArray; BaseName: String; RequiredSHA256OfFile: String; MaxRetries: Integer): RetryableDownloadResult;
begin
  Result := RetryableDownload(DownloadPage, URLs, BaseName, RequiredSHA256OfFile, MaxRetries, CustomMessage('VideoDownloadErrorTitle'), CustomMessage('VideoDownloadErrorRetryPrompt'));
end;

function WZHandleVideoDownload(MaxRetries: Integer): RetryableDownloadResult;
begin
  DownloadPage.Clear;
  if WizardIsComponentSelected('videos\english_hq') then
  begin
    Result := VideoDownloadWrapper(DownloadPage, ['https://downloads.sourceforge.net/project/warzone2100/warzone2100/Videos/high-quality-en/sequences.wz', 'https://github.com/Warzone2100/wz-sequences/releases/download/v3/high-quality-en-sequences.wz'], 'sequences.wz', '', MaxRetries);
  end
  else if WizardIsComponentSelected('videos\english_std') then
  begin
    Result := VideoDownloadWrapper(DownloadPage, ['https://downloads.sourceforge.net/project/warzone2100/warzone2100/Videos/standard-quality-en/sequences.wz', 'https://github.com/Warzone2100/wz-sequences/releases/download/v3/standard-quality-en-sequences.wz'], 'sequences.wz', '', MaxRetries);
  end
  else if WizardIsComponentSelected('videos\english_lq') then
  begin
    Result := VideoDownloadWrapper(DownloadPage, ['https://downloads.sourceforge.net/project/warzone2100/warzone2100/Videos/low-quality-en/sequences.wz', 'https://github.com/Warzone2100/wz-sequences/releases/download/v3/low-quality-en-sequences.wz'], 'sequences.wz', '', MaxRetries);
  end
  else
  // If nothing is selected, return success
  Result := DL_Success;
end;

function CanRunProcessThatNeedsPrivs(): Boolean;
begin
  if IsAdminInstallMode then begin
    // Already running in admin install mode - executing the process should succeed
    Result := True;
  end else begin
    // When not running in admin install mode, can only do this if not in silent mode
    Result := not WizardSilent;
  end;
end;

procedure DisplayVideoDownloadFailedMsgBox();
begin
  SuppressibleTaskDialogMsgBox(
    CustomMessage('VideoDownloadFailedTitle'),
    CustomMessage('VideoDownloadFailedText'),   
    mbError,
    MB_OK, [], 0, IDOK);
end;

// Event handlers

function InitializeSetup(): Boolean;
var
  FallbackStartingTargetArch: String;
begin
  // Check if valid target install architecture is set
  if GetCurrentTargetInstallArchitecture() = '' then
  begin
    // Was unable to determine default compatible target architecture - message user
    #if WZ_ARCHITECTURE_X64
    FallbackStartingTargetArch := 'x64';
    #endif
    if not IsValidTargetArchitecture(FallbackStartingTargetArch) then begin
      SuppressibleMsgBox('Unable to determine target architecture, and no fallback.', mbCriticalError, MB_OK, MB_OK);
      Result := False;
      Exit;
    end;
    if SuppressibleMsgBox(FmtMessage(CustomMessage('UnableToDetermineArchPrompt'), [FallbackStartingTargetArch]), mbError, MB_YESNO or MB_DEFBUTTON2, IDNO) = IDNO then
    begin
      Result := False;
      Exit;
    end else begin
      SetTargetInstallArchitecture('x64');
    end;
  end;
  Result := True;
end;

procedure InitializeWizard;
begin
  DownloadPage := CreateDownloadPage(SetupMessage(msgWizardPreparing), SetupMessage(msgPreparingDesc), @OnDownloadProgress);
  DownloadPage.ShowBaseNameInsteadOfUrl := True;
  
  AdvancedOptionsButton := TNewButton.Create(WizardForm);
  AdvancedOptionsButton.Parent := WizardForm;
  AdvancedOptionsButton.Left := WizardForm.ClientWidth - WizardForm.CancelButton.Left - WizardForm.CancelButton.Width;
  AdvancedOptionsButton.Top := WizardForm.CancelButton.Top;
  AdvancedOptionsButton.Width := WizardForm.CancelButton.Width;
  AdvancedOptionsButton.Height := WizardForm.CancelButton.Height;
  AdvancedOptionsButton.Anchors := [akLeft, akBottom];
  AdvancedOptionsButton.Caption := CustomMessage('AdvancedButton');
  AdvancedOptionsButton.OnClick := @AdvancedInstallOptionsButtonOnClick;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID = wpWelcome then begin
    // Make sure the Advanced button is enabled and visible
    AdvancedOptionsButton.Enabled := True;
    AdvancedOptionsButton.Visible := True;
  end;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  if PageID = wpSelectDir then begin
    Log(ExpandConstant('{#SetupSetting("DefaultDirName")}'));
    // If WizardForm.PrevAppDir <> '', setup detected a previous installation
    // If WizardDirValue = WizardForm.PrevAppDir, setup is currently configured to install to the previous installation's directory
    if not IsPortableMode and (WizardForm.PrevAppDir <> '') and (WizardDirValue() = WizardForm.PrevAppDir) then begin
      // In this specific case, skip showing the directory selection page
      Result := True;
      Exit;
    end;
  end;
  if PageID = wpSelectComponents then begin
    PrepComponents();
  end;
  Result := False;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  if CurPageID = wpWelcome then begin
    // Hide the Advanced button (only available on Welcome page)
    AdvancedOptionsButton.Enabled := False;
    AdvancedOptionsButton.Visible := False;
    Result := True;
  end else if CurPageID = wpSelectDir then begin
    PrepComponents();
    Result := True;
  end else if CurPageID = wpReady then begin
    // Show the main download page first to ensure it covers up everything
    DownloadPage.Clear;
    try
      DownloadPage.Show;
      // Download and install the VCRuntime (if selected)
      if WizardIsComponentSelected('vcruntime') then begin
        if CanRunProcessThatNeedsPrivs then begin
          VCRuntime_DownloadAndInstallArch(GetCurrentTargetInstallArchitecture(), CustomMessage('VCRuntimeVerifying'), CustomMessage('VCRuntimeInstalling'), CustomMessage('VCRuntimeWaiting'));
        end else begin
          SuppressibleMsgBox('Unable to install: Visual C++ Runtime Redistributable. Please install manually.', mbInformation, MB_OK, MB_OK);
        end;
      end;
      // Download videos (if selected)
      case WZHandleVideoDownload(1) of
        DL_AbortedByUser: begin
          Result := False; // if video download is stopped by the user, reject going to next screen (so user can back up and disable videos if desired)
          Exit;
        end;
        // Note: The following conditions may also happen on download failure in silent mode.
        //       We want to proceed but warn that videos will need to be downloaded later.
        DL_RetryCancelledByUser:
          DisplayVideoDownloadFailedMsgBox();
        DL_ExhaustedMaxRetries:
          DisplayVideoDownloadFailedMsgBox();
      end;
      Result := True;
    finally
      DownloadPage.Hide;
    end;
  end else
    Result := True;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  PortableConfigFilePath: String;
begin
  if CurStep = ssPostInstall then
  begin
    if isPortableMode then
      // Create the portable config file
      PortableConfigFilePath := AddBackslash(ExpandConstant('{app}')) + '{#WZ_BINDIR}\{#WZ_PORTABLEMODE_CONFIG_FILE_NAME}';
      Log('Creating portable config file: ' + PortableConfigFilePath);
      SaveStringToFile(PortableConfigFilePath, '# A {#WZ_PORTABLEMODE_CONFIG_FILE_NAME} file in the same directory as the warzone2100 executable enables Portable mode.' + #13#10 + '#' + #13#10 + '# All Warzone 2100 user data will be saved in a subfolder of the directory that contains the warzone2100 executable.' + #13#10, False);
  end;
end;