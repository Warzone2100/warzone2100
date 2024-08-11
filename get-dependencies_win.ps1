# Optionally, specify a VCPKG_BUILD_TYPE
# This will be used to modified the current triplet (once vcpkg is downloaded)
param([string]$VCPKG_BUILD_TYPE = "")

############################

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

# Copy Visual Studio-specific config file templates from "platforms\windows" directory to the repo root
If ( -not (Test-Path (Join-Path "$($ScriptRoot)" "CMakeSettings.json") ) )
{
	Write-Output "Copying template: CMakeSettings.json"
	Copy-Item (Join-Path "$($ScriptRoot)" "platforms\windows\CMakeSettings.json") -Destination "$($ScriptRoot)"
}
If ( -not (Test-Path (Join-Path "$($ScriptRoot)" "launch.vs.json") ) )
{
	Write-Output "Copying template: launch.vs.json"
	Copy-Item (Join-Path "$($ScriptRoot)" "platforms\windows\launch.vs.json") -Destination "$($ScriptRoot)"
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
git reset --hard origin/master;
.\bootstrap-vcpkg.bat;

$triplet = "x86-windows"; # vcpkg default
If (-not ([string]::IsNullOrEmpty($env:VCPKG_DEFAULT_TRIPLET)))
{
	$triplet = "$env:VCPKG_DEFAULT_TRIPLET";
}

function WZ-Prepare-Vcpkg-Triplet($triplet, $VCPKG_BUILD_TYPE, $tripletOverlayFolder)
{
	If (($triplet.Contains("mingw")) -or (-not ([string]::IsNullOrEmpty($VCPKG_BUILD_TYPE))))
	{
		# Need to create a copy of the triplet and modify it
		$tripletFile = "triplets\$($triplet).cmake";
		If (!(Test-Path $tripletFile -PathType Leaf))
		{
			$tripletFile = "triplets\community\$($triplet).cmake";
			If (!(Test-Path $tripletFile -PathType Leaf))
			{
				Write-Error "Unable to find VCPKG_DEFAULT_TRIPLET: $env:VCPKG_DEFAULT_TRIPLET";
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
		$env:VCPKG_OVERLAY_TRIPLETS = "$tripletOverlayFolder";
	}
	
	return $true;
}

WZ-Prepare-Vcpkg-Triplet "$triplet" "$VCPKG_BUILD_TYPE" "$tripletOverlayFolder";

If (-not ([string]::IsNullOrEmpty($env:VCPKG_DEFAULT_HOST_TRIPLET)))
{
	WZ-Prepare-Vcpkg-Triplet "$env:VCPKG_DEFAULT_HOST_TRIPLET" "" "$tripletOverlayFolder";
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
