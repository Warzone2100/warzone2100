function Import-VCVarsEnv() {
  param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string] $vsInstallationPath,
    [Parameter(Position = 1)]
    [string] $args = ""
  )

  if ($vsInstallationPath -and (Test-Path "${vsInstallationPath}\Common7\Tools\vsdevcmd.bat")) {
    & "${env:COMSPEC}" /s /c "`"${vsInstallationPath}\Common7\Tools\vsdevcmd.bat`" $args -no_logo && set" | foreach-object {
      $name, $value = $_ -split '=', 2
      set-content env:\"$name" $value
    }
  }

  return $result
}
