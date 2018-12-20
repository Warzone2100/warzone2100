# Optionally, specify a VCPKG_BUILD_TYPE
# This will be used to modified the current triplet (once vcpkg is downloaded)
param([string]$VCPKG_BUILD_TYPE = "")

############################

# To ensure reproducible builds, pin to a specific vcpkg commit
$VCPKG_COMMIT_SHA = "99b06bba5d86a6982f2491d9f4ea7ef637ff3747";

# To ensure the proper dump_syms.exe is downloaded, specify the commit + hash
$DUMP_SYMS_EXE_COMMIT = "aebee55695eeb40d788f5421bf32eaaa7227aba0";
$DUMP_SYMS_EXE_SHA512 = "AA88547EC486077623A9026EFBC39D7B51912781FDB2C7C6AF5A38165110579EFF9EC04E528D4FDBA7F492A039D94A735ACCAE47074B6E0242855403B609E63E";

############################

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
	Write-Output "Skipping git clone for vcpkg (local copy already exists)";
}
pushd vcpkg;
git reset --hard $VCPKG_COMMIT_SHA;
.\bootstrap-vcpkg.bat;

If (-not ([string]::IsNullOrEmpty($VCPKG_BUILD_TYPE)))
{
	# Add VCPKG_BUILD_TYPE to the specified triplet
	$triplet = "x86-windows"; # vcpkg default
	If (-not ([string]::IsNullOrEmpty($env:VCPKG_DEFAULT_TRIPLET)))
	{
		$triplet = "$env:VCPKG_DEFAULT_TRIPLET";
	}
	$tripletFile = "triplets\$($triplet).cmake";
	$setString = Select-String -Quiet -Pattern "set(VCPKG_BUILD_TYPE `"$VCPKG_BUILD_TYPE`")" -SimpleMatch -Path $tripletFile;
	if (-not $setString)
	{
		Add-Content -Path $tripletFile -Value "`r`nset(VCPKG_BUILD_TYPE `"$VCPKG_BUILD_TYPE`")";
	}
}

$vcpkg_succeeded = -1;
$vcpkg_attempts = 0;
While (($vcpkg_succeeded -ne 0) -and ($vcpkg_attempts -le 2))
{
	.\vcpkg install physfs harfbuzz libiconv libogg libtheora libvorbis libpng openal-soft sdl2 glew freetype gettext zlib;
	$vcpkg_succeeded = $LastExitCode;
	$vcpkg_attempts++;
}
If ($vcpkg_succeeded -ne 0)
{
	Write-Error "vcpkg install failed ($vcpkg_attempts attempts)";
}
popd;

# Download google-breakpad's dump_syms.exe (if necessary)
$dump_syms_path = $(Join-Path (pwd) dump_syms.exe);

If (!(Test-Path $dump_syms_path -PathType Leaf) -or !((Get-FileHash -Path "$dump_syms_path" -Algorithm SHA512).Hash -eq $DUMP_SYMS_EXE_SHA512))
{
	Write-Output "Downloading dump_syms.exe ...";

	# Unfortunately, there does not currently appear to be any way to download the raw file from chromium.googlesource.com
	# Instead, we have to download the Base64-encoded contents of the file and then decode them
	$dump_syms_b64_path = $(Join-Path (pwd) dump_syms_exe.b64);
	Invoke-WebRequest "https://chromium.googlesource.com/breakpad/breakpad/+/$DUMP_SYMS_EXE_COMMIT/src/tools/windows/binaries/dump_syms.exe?format=TEXT" -OutFile "$dump_syms_b64_path"
	$base64string = Get-Content -Raw "$dump_syms_b64_path"
	[IO.File]::WriteAllBytes("$dump_syms_path", [Convert]::FromBase64String($base64string));
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
