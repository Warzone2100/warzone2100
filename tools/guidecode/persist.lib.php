<?php

/****************************************************************
 ** Persistence library                                        **
 ** Version 1.0 RC8                                            **
 ** By Guangcong Luo - http://en.wikipedia.org/wiki/User:Zarel **
 ** released under CC0 and public domain                       **
 ****************************************************************
 *
 * Meant for PHP 4.0.4 and up. I've tried to make it work in
 * PHP 3, but I've never tested it, so use it in PHP 3 at
 * your own risk.
 *
 **[ Description ]***********************************************
 *
 * Creates a new global variable called $_PERSIST, stored in
 * persist.inc.php . If you call persist_update(),
 * persist.inc.php will be updated to store the new
 * value of $_PERSIST.
 *
 * Since you'll probably want to store more than one variable, use
 * $_PERSIST as an array.
 *
 * Make sure PHP has permissions to edit persist.inc.php (If all else
 * fails, CHMOD to 777).
 *
 * The location and name of persist.inc.php, as well as the name of
 * $_PERSIST, can be set either before persist.lib.php is called or
 * below.
 *
 * Example code:
 * <?php
 *  $persist_path = 'includes/custom.inc.php';
 *  $persist_name = 'CUSTOM';
 *  include 'persist.lib.php';
 *  echo $CUSTOM['value'];
 *  $CUSTOM['value'] = 'New value';
 *  persist_update();
 *  echo $CUSTOM['value'];
 * ?>
 *
 * If a script does not need to update the value of $_PERSIST, it can
 * simply include persist.inc.php instead of persist.lib.php .
 *
 **[ Function reference ]****************************************
 *
 *  persist_update()
 *   Updates stored $_PERSIST.
 *
 ****************************************************************/

if (!isset($persist_name) || !$persist_name)
// $persist_name is the name of the variable you want to persist.
// Do not add $ before it; it will be added automatically.
// Defaults to _PERSIST (Which would be the variable $_PERSIST)
	$persist_name = '_PERSIST';

if (!isset($persist_path) || !$persist_path)
// $persist_path is URL of whatever file contains the data you want to
// persist.
// Defaults to persist.inc.php in the directory containing
// persist.lib.php (as opposed to the directory the script is running
// in).
	$persist_path = substr(__FILE__,0,strrpos(__FILE__,'/')+1).strtolower(substr($persist_name,0,1)==='_'?substr($persist_name,1):$persist_name).'.inc.php';

@include_once $persist_path;

function persist_update($name='', $path='')
// Updates $_PERSIST
{
	global $persist_name,$persist_path;
	if (!$name) $name = $persist_name;
	if (!$path && $name == $persist_name) $path = $persist_path;
	if (!$path || !file_exists($path)) $path = substr(__FILE__,0,strrpos(__FILE__,'/')+1).strtolower(substr($name,0,1)==='_'?substr($name,1):$name).'.inc.php';
	// Open persist.inc.php and start editing it
	if (!is_writable($path)) return false;
	$res = fopen($path,"w");
	if (!$res) return false;
	fwrite($res,"<?php\n\$".$name." = ".persist_tophp($GLOBALS[$name]).";\n?>\n");
	fclose($res);
	return true;
}

function persist_tophp($var, $pre='')
// Recursive function to create array
// It really needs a better name
{
	if (is_null($var)) // NULL
		return 'NULL';
	if (is_bool($var)) // Boolean
		return ($var?'TRUE':'FALSE');
	if (is_int($var) || is_float($var)) // Number
		return ''.$var;
	if (is_string($var)) // String
		return "'".php_escape($var)."'";
	if (is_array($var)) //Array
	{
		if (empty($var)) return 'array()';
		$buf = "array(\n";
		$nleft = count($var); $i = -1; reset($var);
		// Recurse (Whee!)
		while (($cur = each($var)) !== FALSE)
		{
			$buf .= $pre."\t";
			if (!is_int($cur[0]))
			{	$buf .= "'".php_escape($cur[0])."' => ";
				$i = FALSE;
			}
			else if ($i===FALSE || $cur[0] != ++$i) 
			{	$buf .= $cur[0].' => ';
				$i = ($i!==FALSE&&$cur[0]>$i?$cur[0]:FALSE);
			}
			$buf .= persist_tophp($cur[1], $pre."\t");
			if (--$nleft) $buf .= ',';
			$buf .= "\n";
		}
		return $buf.$pre.')';
	}
	return "unserialize('".php_escape(serialize($var))."')";
}

function php_escape($str)
{
	return strtr($str,array("\\" => "\\\\", "'" => "\\'"));
}

if (isset($_REQUEST['forceupdate'])) if (persist_update()) echo 'SUCCESS'; else echo 'ERROR';

?>