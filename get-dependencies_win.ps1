# Optionally, specify a VCPKG_BUILD_TYPE
# This will be used to modified the current triplet (once vcpkg is downloaded)
param([string]$VCPKG_BUILD_TYPE = "")

############################

# To ensure reproducible builds, pin to a specific vcpkg commit
$VCPKG_COMMIT_SHA = "7f59e0013648f0dd80377330b770b414032233cb";

# WZ Windows features (for vcpkg install)
$VCPKG_INSTALL_FEATURES = @()
If ((-not ([string]::IsNullOrEmpty($env:VULKAN_SDK))) -and (Test-Path $env:VULKAN_SDK -PathType Container))
{
	Write-Output "VULKAN_SDK environment variable detected - configuring WZ with Vulkan support";
	$VCPKG_INSTALL_FEATURES += "vulkan"
}
Else {
	Write-Output "VULKAN_SDK not detected - configuring WZ *without* Vulkan support";
}

# To ensure the proper dump_syms.exe is downloaded, specify the commit + hash
$DUMP_SYMS_EXE_COMMIT = "aebee55695eeb40d788f5421bf32eaaa7227aba0";
$DUMP_SYMS_EXE_SHA512 = "AA88547EC486077623A9026EFBC39D7B51912781FDB2C7C6AF5A38165110579EFF9EC04E528D4FDBA7F492A039D94A735ACCAE47074B6E0242855403B609E63E";

############################

function Get-ScriptDirectory
{
	$ScriptRoot = ""

	Try
	{
		$commandPath = Get-Variable -Name PSCommandPath -ValueOnly -ErrorAction Stop
		$ScriptRoot = Split-Path -Parent $commandPath
	}
	Catch
	{
		$scriptInvocation = (Get-Variable MyInvocation -Scope 1).Value
		$ScriptRoot = Split-Path $scriptInvocation.MyCommand.Path
	}

	return $ScriptRoot
}

$ScriptRoot = Get-ScriptDirectory;
Write-Output "ScriptRoot=$($ScriptRoot)"

# Copy Visual Studio-specific config file templates from "win32" directory to the repo root
If ( -not (Test-Path (Join-Path "$($ScriptRoot)" "CMakeSettings.json") ) )
{
	Write-Output "Copying template: CMakeSettings.json"
	Copy-Item (Join-Path "$($ScriptRoot)" "win32\CMakeSettings.json") -Destination "$($ScriptRoot)"
}
If ( -not (Test-Path (Join-Path "$($ScriptRoot)" "launch.vs.json") ) )
{
	Write-Output "Copying template: launch.vs.json"
	Copy-Item (Join-Path "$($ScriptRoot)" "win32\launch.vs.json") -Destination "$($ScriptRoot)"
}

# Create build-dir vcpkg overlay folders
$tripletOverlayFolder = (Join-Path (pwd) vcpkg_overlay_triplets)
if (!(Test-Path -path $tripletOverlayFolder)) {New-Item $tripletOverlayFolder -Type Directory}

# Download & build vcpkg (+ dependencies)
If ( -not (Test-Path (Join-Path (pwd) vcpkg\.git) -PathType Container) )
{
	# Clone the vcpkg repo
	Write-Output "Cloning https://github.com/Microsoft/vcpkg.git ...";
	git clone -q https://github.com/Microsoft/vcpkg.git;
}
Else
{
	# On CI (for example), the vcpkg directory may have been cached and restored
	# Fetch origin updates
	Write-Output "Fetching origin updates for vcpkg (local copy already exists)";
	pushd vcpkg;
	git fetch origin;
	popd;
}
pushd vcpkg;
git reset --hard $VCPKG_COMMIT_SHA;
.\bootstrap-vcpkg.bat;

$triplet = "x86-windows"; # vcpkg default
If (-not ([string]::IsNullOrEmpty($env:VCPKG_DEFAULT_TRIPLET)))
{
	$triplet = "$env:VCPKG_DEFAULT_TRIPLET";
}

If (($triplet.Contains("mingw")) -or (-not ([string]::IsNullOrEmpty($VCPKG_BUILD_TYPE))))
{
	# Need to create a copy of the triplet and modify it
	$tripletFile = "triplets\$($triplet).cmake";
	If (!(Test-Path $tripletFile -PathType Leaf))
	{
		$tripletFile = "triplets\community\$($triplet).cmake";
		If (!(Test-Path $tripletFile -PathType Leaf))
		{
			Write-Error "Unable to find VCPKG_DEFAULT_TRIPLET: $env:VCPKG_DEFAULT_TRIPLET"
		}
	}
	Copy-Item "$tripletFile" -Destination "$tripletOverlayFolder"
	$tripletFileName = Split-Path -Leaf "$tripletFile"
	$overlayTripletFile = "$tripletOverlayFolder\$tripletFileName"
	If ($triplet.Contains("mingw"))
	{
		# A fix for libtool issues with mingw-clang
		Add-Content -Path $overlayTripletFile -Value "`r`nlist(APPEND VCPKG_CONFIGURE_MAKE_OPTIONS `"lt_cv_deplibs_check_method=pass_all`")";

		# Build with pdb debug symbols (mingw-clang)
		Add-Content -Path $overlayTripletFile -Value "`r`nstring(APPEND VCPKG_CXX_FLAGS `" -gcodeview -g `")`r`nstring(APPEND VCPKG_C_FLAGS `" -gcodeview -g `")`r`nstring(APPEND VCPKG_LINKER_FLAGS `" -Wl,-pdb= `")";
	}
	If (-not ([string]::IsNullOrEmpty($VCPKG_BUILD_TYPE)))
	{
		Add-Content -Path $overlayTripletFile -Value "`r`nset(VCPKG_BUILD_TYPE `"$VCPKG_BUILD_TYPE`")";
	}
	# Setup environment variable so vcpkg uses the overlay triplets folder
	$env:VCPKG_OVERLAY_TRIPLETS = "$tripletOverlayFolder"
}

# Patch vcpkg_copy_pdbs for mingw support
$vcpkg_copy_pdbs_patch = (Join-Path "$($ScriptRoot)" ".ci\vcpkg\patches\scripts\cmake\vcpkg_copy_pdbs.cmake");
$vcpkg_copy_pdbs_dest = (Join-Path (pwd) "scripts\cmake");
Copy-Item "$vcpkg_copy_pdbs_patch" -Destination "$vcpkg_copy_pdbs_dest"

popd;

$overlay_ports_path = (Join-Path "$($ScriptRoot)" ".ci\vcpkg\overlay-ports");
$additional_vcpkg_flags = @("--x-no-default-features");
$VCPKG_INSTALL_FEATURES | ForEach-Object { $additional_vcpkg_flags += "--x-feature=${PSItem}" };

$vcpkg_succeeded = -1;
$vcpkg_attempts = 0;
Write-Output "vcpkg install --x-manifest-root=$($ScriptRoot) --x-install-root=.\vcpkg_installed\ --overlay-ports=$($overlay_ports_path) $additional_vcpkg_flags";

$vcpkg_path = (Join-Path (pwd) "vcpkg");
$vcpkg_executable = (Join-Path "$($vcpkg_path)" "vcpkg.exe");
While (($vcpkg_succeeded -ne 0) -and ($vcpkg_attempts -le 2))
{
	& $($vcpkg_executable) install --vcpkg-root=$($vcpkg_path) --x-manifest-root=$($ScriptRoot) --x-install-root=.\vcpkg_installed\ --overlay-ports=$($overlay_ports_path) $additional_vcpkg_flags;
	$vcpkg_succeeded = $LastExitCode;
	$vcpkg_attempts++;
}
If ($vcpkg_succeeded -ne 0)
{
	Write-Error "vcpkg install failed ($vcpkg_attempts attempts)";
	exit 1
}

# Download google-breakpad's dump_syms.exe (if necessary)
$dump_syms_path = $(Join-Path (pwd) dump_syms.exe);

If (!(Test-Path $dump_syms_path -PathType Leaf) -or !((Get-FileHash -Path "$dump_syms_path" -Algorithm SHA512).Hash -eq $DUMP_SYMS_EXE_SHA512))
{
	Write-Output "Downloading dump_syms.exe ...";

	# Unfortunately, there does not currently appear to be any way to download the raw file from chromium.googlesource.com
	# Instead, we have to download the Base64-encoded contents of the file and then decode them
	$dump_syms_b64_response = Invoke-WebRequest "https://chromium.googlesource.com/breakpad/breakpad/+/$DUMP_SYMS_EXE_COMMIT/src/tools/windows/binaries/dump_syms.exe?format=TEXT"
	[IO.File]::WriteAllBytes("$dump_syms_path", [Convert]::FromBase64String($dump_syms_b64_response.Content));
	$dump_syms_hash = Get-FileHash -Path "$dump_syms_path" -Algorithm SHA512;
	If ($dump_syms_hash.Hash -eq $DUMP_SYMS_EXE_SHA512) {
		Write-Output "Successfully downloaded dump_syms.exe";
	}
	Else {
		Write-Error "The downloaded dump_syms.exe hash '$($dump_syms_hash.Hash)' does not match the expected hash: '$DUMP_SYMS_EXE_SHA512'";
	}
}
Else
{
	Write-Output "dump_syms.exe already exists";
}
