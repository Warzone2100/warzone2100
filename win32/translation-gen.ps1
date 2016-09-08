# Input is po file
function GeneratePath ($dirname) {
	$var = "../../locale/" + $dirname +"/LC_MESSAGES/"
	New-Item -Force -ItemType Directory -Path $var
	$pofile = "../../po/" + $dirname + ".po"
	$mofile = $var + "warzone2100.mo"
	$cmdstr = "..\packages\Gettext.Tools.0.19.8.1\tools\bin\msgfmt.exe " + $pofile + " -o " + $mofile
	cmd /c $cmdstr
}

Get-ChildItem ../../po/*.po | foreach {$_.Name} | foreach {$_.substring(0, $_.Length -3)} | foreach { GeneratePath $_}
