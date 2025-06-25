function Build-WinCombinedArchDir {
  param(
    [Parameter(Mandatory=$true)]
    [string]$InputArchivesDir,
  
    [Parameter(Mandatory=$true)]
    [string]$TmpExtractDir,
  
    [Parameter(Mandatory=$true)]
    [string]$BuildDir
  )

  $InputArchivesDir = $(Resolve-Path -Path "$InputArchivesDir")
  $TmpExtractDir = $(Resolve-Path -Path "$TmpExtractDir")
  $BuildDir = $(Resolve-Path -Path "$BuildDir")

  echo "InputArchivesDir=$InputArchivesDir"
  echo "TmpExtractDir=$TmpExtractDir"
  echo "BuildDir=$BuildDir"

  $firstFile = "warzone2100_win_x64_archive.zip" # Default to the x64 archive for the non-binary (data) files
  $Files = Get-ChildItem -Path "${InputArchivesDir}" -Filter "warzone2100_win_*_archive.zip" | Sort-Object { if ($_.Name -eq $firstFile) { 0 } else { 1 } }, Name

  if ($Files.Count -eq 0) {
    throw "No matching archive zips found in: ${InputArchivesDir}"
  }

  $IsFirstAsset = $true
  foreach ($file in $Files) {
    Write-Host "Processing file: $($file.FullName)"
    $Release_Asset_Name = $file.Name
    $segments = $Release_Asset_Name -split "_"
    $Release_Asset_Arch = $segments[2]
    
    # Extract the ZIP file to a temporary location
    echo "::group::Extract Archive ($Release_Asset_Arch)"
    $TmpExtractLocation = "${TmpExtractDir}\tmp_extract"
    if (Test-Path "$TmpExtractLocation") { Remove-Item "$TmpExtractLocation" -Recurse -Force }
    New-Item -ItemType Directory -Force -Path "$TmpExtractLocation"
    Expand-Archive -LiteralPath "$($file.FullName)" -DestinationPath "$TmpExtractLocation"
    echo "::endgroup::"
    
    # Copy the necessary parts to the staging directory
    echo "::group::Build File + Dir Lists ($Release_Asset_Arch)"
    if ($IsFirstAsset) {
      # Copy the entirety of the archive (the first architecture's archive is used to stage all data and other files as well)
      $fileList = @(Get-ChildItem -Path "${TmpExtractLocation}" -File -Recurse)
      $dirList = @(Get-ChildItem -Path "${TmpExtractLocation}" -Directory -Recurse)
    }
    else {
      # Only copy the bin folder contents into an architecture-specific subdirectory
      $fileList = @(Get-ChildItem -Path (Join-Path "${TmpExtractLocation}" "bin") -File -Recurse)
      $dirList = @(Get-Item -Path (Join-Path "${TmpExtractLocation}" "bin"))
      $dirList += @(Get-ChildItem -Path (Join-Path "${TmpExtractLocation}" "bin") -Directory -Recurse)
    }
    echo "::endgroup::"
    
    echo "::group::Create Dirs ($Release_Asset_Arch)"
    foreach ($dirPath in $dirList)
    {
      $destinationDirPath = $($dirPath.FullName).Replace("${TmpExtractLocation}", "${BuildDir}")
      if ($destinationDirPath.StartsWith($(Join-Path "${BuildDir}" "bin"))) {
        # Move the contents of the bin directory into a subdirectory by architecture
        $destinationDirPath = ($destinationDirPath).Replace($(Join-Path "${BuildDir}" "bin"), $(Join-Path "${BuildDir}" -ChildPath "bin" | Join-Path -ChildPath "${Release_Asset_Arch}"))
      }
      echo "Creating: $destinationDirPath"
      New-Item "$destinationDirPath" -ItemType Directory -ea SilentlyContinue | Out-Null
    }
    echo "::endgroup::"
    echo "::group::Stage Files ($Release_Asset_Arch)"
    foreach ($file in $fileList)
    {
      $filePath = $(Split-Path $($file.FullName))
      $destinationPath = $filePath.Replace("${TmpExtractLocation}", "${BuildDir}")
      if ($filePath.StartsWith($(Join-Path "${TmpExtractLocation}" "bin"))) {
        # Move the contents of the bin directory into a subdirectory by architecture
        $destinationPath = $filePath.Replace($(Join-Path "${TmpExtractLocation}" "bin"), $(Join-Path "${BuildDir}" -ChildPath "bin" | Join-Path -ChildPath "${Release_Asset_Arch}"))
      }
      echo "Staging: $destinationDirPath\$($file.Name)"
      Move-Item -Path "$($file.FullName)" -Destination "$destinationPath" -Force -ErrorAction Stop
    }
    echo "::endgroup::"

    if ($IsFirstAsset) {
      # Verify that all files were moved - should only be empty folders remaining
      $fileList = @(Get-ChildItem -Path "$($TmpExtractLocation)" -File -Recurse)
      if ($fileList.count > 0) {
        Write-Error "Failed - there are files remaining that weren't moved!"
        Write-Output "$fileList"
        exit 1
      }
    }
    
    # Remove temporary extracted folder
    Write-Host "Deleting temporary extraction folder: '$($TmpExtractLocation)'"
    Remove-Item "$TmpExtractLocation" -Recurse -Force -ErrorAction Stop

    $IsFirstAsset = $false
  }
  
  Write-Host "Finished staging combined archive files in: ${BuildDir}"
}
