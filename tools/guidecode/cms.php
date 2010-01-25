<?php

include_once 'r.inc.php';
include_once 'guide.inc.php';
include_once 'persist.lib.php';

if (!$is_admin) die('Access denied.');

$pane = $_REQUEST['p'];

function 

if ($_REQUEST['act'])
{
	switch ($_REQUEST['act'])
	{
	case 'newfile':
		//
		break;
	case 'newdir':
		break;
	case 'rm':
	}
}

if (!$pane)
{
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Frameset//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-frameset.dtd">
<html xml:lang="en" lang="en" xmlns="http://www.w3.org/1999/xhtml">
  <head>
    <title>cms</title>
  </head>
  <frameset cols="220,*" border="0">
    <frame name="s" src="cms.php?p=s" frameborder="0" marginwidth="0" marginheight="0" noresize="noresize" scrolling="auto" />
    <frame name="m" src="cms.php?p=m" frameborder="0" marginwidth="0" marginheight="0" noresize="noresize" scrolling="auto" />
    <noframes>
      Error: Frames-less version not yet supported. Please check back later.
    </noframes>
  </frameset>
</html>
<?php
}
else if ($pane == 's')
{
?>
<body><strong>List</strong>
<?php
function genlist($items, $loc='')
{
	$out = '<ul>';
	if ($id) $loc .= $id.'/';
	foreach ($items as $item)
	{
		if ($item['type'] == 'dir')
			$out .= '<li>'.$item['name'].genlist($item, $loc).'</li>';
		else if ($item['type'] == 'page')
			$out .= '<li><a href="textedit.php?id='.$loc.$id.'">'.$item['name'].'</a></li>';
	}
	$out .= '<li><a href="cms.php?p=m&id='.$loc.'">New</a></li>';
	$out .= '</ul>';
	return $out;
}
?>
</body>
<?php
}
else if ($pane == 'm')
{
?>
CID: <?=$id?>
<form action="cms.php?p=s&id=<?=$id?>" target="s" method="post">
New file.<br />
ID: <input type="text" name="id" id="id" /><br />
Name: <input type="text" name="name" id="name" /><br />
<input type="hidden" name="act" value="newfile" />
<input type="submit" value="New file" />
</form>
<form action="cms.php?p=m&id=<?=$id?>" method="post">
New dir.<br />
ID: <input type="text" name="id" id="id" /><br />
Name: <input type="text" name="name" id="name" /><br />
<input type="hidden" name="act" value="newdir" />
<input type="submit" value="New dir" />
</form>
<form action="cms.php?p=m&id=<?=$id?>" method="post">
Remove file/dir.<br />
ID: <input type="text" name="id" id="id" /><br />
<input type="hidden" name="act" value="rm" />
<input type="submit" value="Remove" />
</form>
<?php
}
?>