<?php

// Warzone Guide main library

// configuration

$adminpass = 'INSERT PASSWORD HERE';
// adminpass must not contain ':', and differences in capitalization and whitespace will be ignored

// code

if (substr($_SERVER['REQUEST_URI'],0,9) === '/r/r.php?')
{
	header('HTTP/1.1 301 Moved Permanently');
	header('Location: http://guide.wz2100.net/r/'.substr($_SERVER['REQUEST_URI'],9));
	die();
}
if (substr($_SERVER['REQUEST_URI'],0,9) === '/c/c.php?')
{
	header('HTTP/1.1 301 Moved Permanently');
	header('Location: http://guide.wz2100.net/c/'.substr($_SERVER['REQUEST_URI'],9));
	die();
}
if (substr($_SERVER['REQUEST_URI'],0,9) === '/w/w.php?')
{
	header('HTTP/1.1 301 Moved Permanently');
	header('Location: http://guide.wz2100.net/w/'.substr($_SERVER['REQUEST_URI'],9));
	die();
}
if (substr($_SERVER['REQUEST_URI'],0,9) === '/d/d.php?')
{
	header('HTTP/1.1 301 Moved Permanently');
	header('Location: http://guide.wz2100.net/d/'.substr($_SERVER['REQUEST_URI'],9));
	die();
}
if (substr($_SERVER['REQUEST_URI'],0,9) === '/b/b.php?')
{
	header('HTTP/1.1 301 Moved Permanently');
	header('Location: http://guide.wz2100.net/b/'.substr($_SERVER['REQUEST_URI'],9));
	die();
}
if (substr($_SERVER['REQUEST_URI'],0,11) === '/bp/bp.php?')
{
	header('HTTP/1.1 301 Moved Permanently');
	header('Location: http://guide.wz2100.net/bp/'.substr($_SERVER['REQUEST_URI'],11));
	die();
}

$icons = array(
	'commandturretmkii' => 'commandturret',
	'commandturretmkiii' => 'commandturret',
	'commandturretmkiv' => 'commandturret',
	'commandturretii' => 'commandturret',
	'commandturretiii' => 'commandturret',
	'commandturretiv' => 'commandturret',

	'vtolclusterbombsbay' => 'vtolclusterbombbay',

	'cannon' => 'lightcannon',

	'minirocket' => 'minirocketpod',
	'mrlemplacement' => 'minirocketbattery',
	'archangelmissileemplacement' => 'archangelmissilebattery',
	'angelmissileemplacement' => 'angelmissilebattery',
	'seraphmissilearray' => 'angelmissile',
	'seraphmissilebattery' => 'angelmissilebattery',

	'transport' => 'cyborgtransport',
	'transportbody' => 'cyborgtransport',

	'hypervelocitycannonemplacement' => 'hpvcannonemplacement',
	'hypervelocitycannonhardpoint' => 'hpvcannonhardpoint',

	'cyborgchaingun1ground' => 'heavygunner',
	'cyborgpropulsion' => 'heavygunner',

	'empmortar' => 'empmortarpit',
	'massdriver' => 'massdriverfortress',
	'plasmacannonemplacement' => 'heavylaseremplacement',
	'empcannontower' => 'empmissilehardpoint',

	'hardcretecornerwall' => 'hardcretewall',
	'engineering' => 'truck',
);

$imgroot = '';
if (is_dir('../icon/')) $imgroot = '../';

function geticon($id, $type=false)
{
	global $icons, $imgroot, $wz_research;
	
	if ((!$type || $type == 'w') && file_exists($imgroot.'icon/w/'.$id.'.gif'))
		return $imgroot.'icon/w/'.$id.'.gif';
	if ((!$type || $type == 'c') && file_exists($imgroot.'icon/c/'.$id.'.gif'))
		return $imgroot.'icon/c/'.$id.'.gif';
	if ((!$type || $type == 'bp') && file_exists($imgroot.'icon/bp/'.$id.'.gif'))
		return $imgroot.'icon/bp/'.$id.'.gif';
	if ((!$type || $type == 'd') && file_exists($imgroot.'icon/d/'.$id.'.gif'))
		return $imgroot.'icon/d/'.$id.'.gif';
	if ((!$type || $type == 'b') && file_exists($imgroot.'icon/b/'.$id.'.gif'))
		return $imgroot.'icon/b/'.$id.'.gif';
	if ($type)
	{
		if ($icon = geticon($id)) return $icon;
		if ($type == 'r' && $wz_research[$id]['results'][0][1]) return geticon($wz_research[$id]['results'][0][1]);
	}
	if (isset($icons[$id]))
		return geticon($icons[$id]);
	if (substr($id,0,4) == 'vtol')
		return geticon(substr($id,4));
	if (substr($id,-1) == '2')
		return geticon(substr($id,0,-1));
	return false;
}
function iconimg($id, $type=false, $prefix='../')
{
	if ($icon = geticon($id, $type))
		return '<img src="'.$prefix.$icon.'" alt="" style="vertical-align:middle;" /> ';
	return '';
}

$isadmin = false;
$isauth = false;
if (substr($_COOKIE['name'],-strlen($adminpass)-1)===':'.$adminpass)
{
	$isadmin = $isauth = true;
	$_COOKIE['name'] = substr($_COOKIE['name'],0,-strlen($adminpass)-1);
}
else if ($_REQUEST['captcha'] === $adminpass)
	$isadmin = $isauth = true;
else if (substr($_COOKIE['name'],-3) == ':mg')
{
	$isauth = true;
	$_COOKIE['name'] = substr($_COOKIE['name'],0,-3);
}
else if ($_COOKIE['name']) $_POST['forget'] = true;

@include_once 'cache/research.inc.php';
@include_once 'cache/weapons.inc.php';
@include_once 'cache/weapontable.inc.php';
@include_once 'cache/wsubclasses.inc.php';
@include_once 'cache/functions.inc.php';
@include_once 'cache/structures.inc.php';
@include_once 'cache/bodies.inc.php';
@include_once 'cache/propulsions.inc.php';
@include_once 'cache/templates.inc.php';
@include_once 'persist.lib.php';
@include_once 'guide.inc.php';

$idr = array_keys($_GET);
$id = '';
if (!$_GET[$idr[0]])
{
	$id = $idr[0];
}
else if ($_GET['w'] && $_GET['b'] && $_GET['p'])
{
	$id = true;
}
if (!$id && $run)
{
	header("HTTP/1.0 404 Not Found");
	include('404.php');
	die();
}
else if ($id && $id !== true && (
	substr($_SERVER['REQUEST_URI'],0,3)=='/b/' && !$wz_structures[$id] ||
	substr($_SERVER['REQUEST_URI'],0,3)=='/d/' && !$wz_structures[$id] ||
	substr($_SERVER['REQUEST_URI'],0,3)=='/w/' && !$wz_weapons[$id] ||
	substr($_SERVER['REQUEST_URI'],0,3)=='/c/' && !$wz_cyborgs[$id] && !$wz_templates[$id] ||
	substr($_SERVER['REQUEST_URI'],0,4)=='/bp/' && !$wz_bodies[$id] && !$wz_propulsions[$id] ||
	substr($_SERVER['REQUEST_URI'],0,3)=='/r/' && !$wz_research[$id]
))
{
	header("HTTP/1.0 404 Not Found");
	include('404.php');
	die();
}
$ar = array();

$dt = array('weaponacc' => 'accuracy', 'weapondam' => 'damage', 'weaponsdam' => 'splash damage', 'weaponfdam' => 'burn damage', 'weaponrof' => 'rate of fire');
$upgrades = array('powerupgrade' => 'power production', 'researchupgrade' => 'research speed', 'repairupgrade' => 'repair facility speed', 'rearmupgrade' => 'VTOL rearming pad speed', 'vehicleconstupgrade' => 'construction speed', 'factoryupgrade' => 'factory production speed', 'cyborgfactoryupgrade' => 'cyborg factory production speed', 'vtolfactoryupgrade' => 'VTOL factory production speed', 'vehiclesensorupgrade' => 'vehicle sensor range', 'vehicleecmupgrade' => 'vehicle ECM range', 'vehicleengine' => 'vehicle engine output', 'vehiclearmor' => 'vehicle kinetic armor', 'vehiclehp' => 'vehicle HP', 'vehiclethermal' => 'vehicle thermal armor', 'cyborgengine' => 'cyborg engine output', 'cyborghp' => 'cyborg HP', 'cyborgarmor' => 'cyborg kinetic armor', 'cyborgthermal' => 'cyborg thermal armor', 'wallarmor' => 'hardcrete kinetic/thermal armor', 'wallhp' => 'hardcrete HP', 'structurearmor' => 'base structure kinetic/thermal armor', 'structurehp' => 'base structure HP', 'structureresist' => 'base structure Nexus link resistance');

$directions = array('direct' => '', 'indirect' => 'indirect ', 'indirecthoming' => 'indirect-homing ', 'homingdirect' => 'homing ', 'erraticdirect' => 'erratic ');
$classes = array('kinetic' => 'kinetic', 'heat' => 'thermal');
$types = array('antiaircraft' => 'anti-air', 'antipersonnel' => 'anti-personnel', 'antitank' => 'anti-tank', 'artilleryround' => 'artillery', 'bunkerbuster' => 'anti-structure', 'flamer' => 'fire', 'allrounder' => 'all-rounder');
$subclasses = array('machinegun' => 'machinegun', 'cannon' => 'cannon', 'flame' => 'flamer', 'mortars' => 'mortar', 'howitzers' => 'howitzer', 'rocket' => 'mini-rocket', 'missile' => 'missile', 'slowrocket' => 'rocket', 'gauss' => 'rail gun', 'energy' => 'laser', 'electronic' => 'electronic weapon', 'command' => 'commander', 'emp' => 'EMP cannon', 'aagun' => 'AA-gun', 'bomb' => 'bomb', 'lassat' => 'laser satellite');
$props = array('legged' => 'Cyborg', 'wheeled' => 'Wheels', 'halftracked' => 'Half-Tracks', 'tracked' => 'Tracks', 'lift' => 'VTOL', 'hover' => 'Hover', 'soft' => 'Soft structures', 'medium' => 'Medium structures', 'hard' => 'Hard structures', 'bunker' => 'Bunkers');
$sensors = array('standard' => 'Sensor', 'indirectcb' => 'Counter-Battery', 'vtolintercept' => 'VTOL Sensor', 'vtolcb' => 'VTOL Counter-Battery', 'super' => 'Sensor, VTOL, CB, VTOL CB', 'radardetector' => 'Sensor Detector', 'uplink' => 'Reveal entire map');
$strengths = array('soft' => 'Soft', 'medium' => 'Medium', 'hard' => 'Hard', 'bunker' => 'Bunker');

$rebalance = false;
if ($wz_weapons['heavycannon']['type'] == 'antiaircraft')
{
	$rebalance = true;
	$types['antiaircraft'] = 'all-rounder';
}

function simplify($name,$array=array())
{
	$name = strtr($name, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", "abcdefghijklmnopqrstuvwxyz");
	$in = ($name[0]=='*');
	$name = preg_replace('/[^a-z0-9]+/','',$name);
	$i = 1;
	if ($name) $id = 'internal-'.$name;
	//if (strpos($wzid,'R-') === 0) $name = 'r-'.$name;
	//else if (strpos($wzid,'Cyborg-Wpn-') !== false || strpos($wzid,'Cyb-') || //strpos($wzid,'Cyborg') !== false) $name = 'c-'.$name;
	$id = $name;
	while (in_array($id,$array)) $id = $name.(++$i);
	return $id;
}

function tourl($id)
{
	if (!$id) return '';
	return $id;
}

/*
 * GENERATOR FUNCTIONS
 */ 

function upg($val, $upg=0)
{
	return round($val*(100+$upg)/100,1);
}
function iupg($val, $upg=0)
{
	return intval($val*(100+$upg)/100);
}
function mktemplate($weapon,$body,$prop)
{
	global $wz_weapons, $wz_bodies, $wz_propulsions;
	if (is_string($weapon)) $weapon = $wz_weapons[$weapon];
	if (is_string($body)) $body = $wz_bodies[$body];
	if (is_string($prop)) $prop = $wz_propulsions[$prop];

	return array(
		'tid' => '',
		'name' => $weapon['name'].' '.$body['name'].' '.$prop['name'],
		'turrets' => array(
			$weapon['wid']
		),
		'body' => $body['bid'],
		'propulsion' => $prop['pid'],
		'brain' => '',
		'construct' => '',
		'ecm' => '',
		'repair' => NULL,
		'sensor' => NULL,
		'prereq' => '',
		'hp' => iupg($body['hp'],$prop['hp']),
		'armor' => $body['armor'],
		'thermal' => $body['thermal'],
		'engine' => $body['engine'],
		'maxspeed' => $prop['maxspeed'],
		'weight' => $weapon['weight']+iupg($body['weight'],$prop['weight']),
		'bp' => $weapon['bp']+iupg($body['bp'],$prop['bp']),
		'price' => $weapon['price']+$body['price']+$prop['price'],
		'aionly' => FALSE
	);
}

function weapontype($weapon, $upg=0)
{
	global $directions, $classes, $types, $subclasses;
	$res = '';
	if ($weapon['splash'])
		$res = 'Splash: '.iupg($weapon['splashdamage'],$upg).($weapon['splashacc']==100?'':' - '.$weapon['splashacc'].'%').' - radius '.round($weapon['splash']/128,1).' | ';
	if ($weapon['burn'])
		$res .= 'Burn: '.iupg($weapon['burndamage'],$upg).' - '.($weapon['burntime']/10).' sec - radius '.round($weapon['burn']/128,1).' | ';
	return $res.$directions[$weapon['direction']].$classes[$weapon['class']].' '.$types[$weapon['type']].' '.$subclasses[$weapon['subclass']];
}
function weapondam($weapon, $upg=0)
{
	if ($weapon['subclass']=='emp' && $weapon['damage']==0)
	{
		return "<abbr class=\"detail\" title=\"".weapontype($weapon, $upg)."\">EMP".($weapon['splash']?'s':'').($weapon['burn']?'f':'')."</abbr>";
	}
	return "<abbr class=\"detail".($weapon['class']=='heat'?' thermal':' kinetic')."\" title=\"".weapontype($weapon, $upg)."\">".iupg($weapon['damage'],$upg)."".($weapon['splash']?'s':'').($weapon['burn']?'f':'').($weapon['subclass']=='emp'?'+EMPs':'')."</abbr>";
}
function weaponrof($weapon, $upg=0)
{
	if ($weapon['numattackruns'])
		return "<abbr class=\"detail\" title=\"".$weapon['nrounds']." round".($weapon['nrounds']==1?'':'s')." every ".round(($weapon['fcooldown']*$weapon['nrounds']+$weapon['cooldown'])*(100-$upg)/1000,1)." sec".($weapon['targetair']?($weapon['targetground']?' | versatile':' | anti-air'):'')."\">".$weapon['nrounds']."<span class=\"smallgrey\">&times;".$weapon['numattackruns']."</span></abbr>";
	return "<abbr class=\"detail\" title=\"".$weapon['nrounds']." round".($weapon['nrounds']==1?'':'s')." every ".round(($weapon['fcooldown']*$weapon['nrounds']+$weapon['cooldown'])*(100-$upg)/1000,1)." sec".($weapon['targetair']?($weapon['targetground']?' | versatile':' | anti-air'):'')."\">".round($weapon['rof']*100/(100-$upg),1)."</abbr>";
}
function templatehp($template, $upg=0)
{
	global $wz_bodies, $wz_propulsions;
	return '<abbr class="detail" title="Body: '.($wz_bodies[$template['body']]['hp']).', Propulsion: '.($wz_bodies[$template['body']]['hp']*$wz_propulsions[$template['propulsion']]['hp']/100).', Turret: '.($template['hp']-$wz_bodies[$template['body']]['hp']*($wz_propulsions[$template['propulsion']]['hp']+100)/100).($upg?', Upgrades: +'.(iupg($template['hp'],$upg)-$template['hp']):'').'">'.iupg($template['hp'],$upg)."</abbr>";
}
function structurehp($structure, $upg=0)
{
	global $wz_weapons;
	if (!$structure['turrets']) return ''.$structure['hp'];
	return '<abbr class="detail" title="Base: '.($structure['hp']-$wz_weapons[$structure['turrets'][0]]['hp']).', Turret: '.($wz_weapons[$structure['turrets'][0]]['hp']).($upg?', Upgrades: +'.(iupg($structure['hp'],$upg)-$structure['hp']):'').'">'.iupg($structure['hp'],$upg)."</abbr>";
}
function bodyarmor($body, $upg1=0, $upg2=0)
{
	//if ($body['body']) $body = $GLOBALS['wz_bodies'][$body['body']];
	return '<span class="armor"><span class="kinetic">'.iupg($body['armor'],$upg1).'</span>/<span class="thermal">'.iupg($body['thermal'],$upg2).'</span></span>';
}
function structurearmor($struct, $upg=0)
{
	//if ($body['body']) $body = $GLOBALS['wz_bodies'][$body['body']];
	return '<span class="armor"><span class="kinetic">'.iupg($struct['armor'],$upg).'</span>/<span class="thermal">'.iupg($struct['armor'],$upg).'</span></span>';
}
function templatespeed($template, $upg=0)
{
	global $wz_propulsions, $wz_proptypes, $wz_bodies;
	$prop = $wz_propulsions[$template['propulsion']];
	if (!$template['weight']) return 0;
	return intval(min(
		$prop['maxspeed'],
		$wz_proptypes[$prop['prop']]['speed']*iupg($wz_bodies[$template['body']]['engine'],$upg)/$template['weight']
	));
}
function weaponupgrades($weapon, $root='')
{
	global $wz_wsubclasses, $wz_research, $wz_functions;
	if (is_string($weapon)) $weapon = $GLOBALS['wz_weapons'][$weapon];
	$tout = '';
	if ($weapon['turret'] == 'weapon' && $wz_wsubclasses[$weapon['subclass']]['upgrades'])
	{
		foreach ($wz_wsubclasses[$weapon['subclass']]['upgrades'] as $upgrade)
		{
			if ($wz_functions[$wz_research[$upgrade]['result'][0][1]]['funcs'][0]['type'] == 'weapondam')
				$tout .= '<li>'.($wz_functions[$wz_research[$upgrade]['result'][0][1]]['funcs'][0]['amt']+100).'% - <a href="'.$root.'r/'.$upgrade.'">'.$wz_research[$upgrade]['name']."</a></li>\n";
		}
		if ($tout) $out .= "<h4 id=\"damupgrades\">Damage upgrades</h4>\n<ul>".$tout."</ul>\n";
		else $out .= "<h4 id=\"damupgrades\">Damage upgrades</h4>\n<p><em>No damage upgrades available.</em></p>\n";
		$tout = '';
		foreach ($wz_wsubclasses[$weapon['subclass']]['upgrades'] as $upgrade)
			if ($wz_functions[$wz_research[$upgrade]['result'][0][1]]['funcs'][0]['type'] == 'weaponrof')
				$tout .= '<li>'.round(10000/(100-$wz_functions[$wz_research[$upgrade]['result'][0][1]]['funcs'][0]['amt']),0).'% - <a href="'.$root.'r/'.$upgrade.'">'.$wz_research[$upgrade]['name']."</a></li>\n";
		if ($tout) $out .= "<h4 id=\"rofupgrades\">ROF upgrades</h4>\n<ul>".$tout."</ul>\n";
		else $out .= "<h4 id=\"rofupgrades\">ROF upgrades</h4>\n<p><em>No ROF upgrades available.</em></p>\n";
		$tout = '';
		foreach ($wz_wsubclasses[$weapon['subclass']]['upgrades'] as $upgrade)
			if ($wz_functions[$wz_research[$upgrade]['result'][0][1]]['funcs'][0]['type'] == 'weaponacc')
				$tout .= '<li>'.($wz_functions[$wz_research[$upgrade]['result'][0][1]]['funcs'][0]['amt']+100).'% - <a href="'.$root.'r/'.$upgrade.'">'.$wz_research[$upgrade]['name']."</a></li>\n";
		if ($tout) $out .= "<h4 id=\"accupgrades\">Accuracy upgrades</h4>\n<ul>".$tout."</ul>\n";
		else $out .= "<h4 id=\"accupgrades\">Accuracy upgrades</h4>\n<p><em>No accuracy upgrades available.</em></p>\n";
	}
	else if ($weapon['turret'] == 'construct')
	{
		$out .= "<ul>";
		foreach ($wz_wsubclasses['construct']['upgrades'] as $upgrade)
			$out .= '<li>'.($wz_functions[$wz_research[$upgrade]['result'][0][1]]['funcs'][0]['amt']+100).'% - <a href="'.$root.'r/'.$upgrade.'">'.$wz_research[$upgrade]['name']."</a></li>\n";
		$out .= "</ul>\n";
	}
	else if ($weapon['turret'] == 'sensor')
	{
		$out .= "<ul>";
		foreach ($wz_wsubclasses['sensor']['upgrades'] as $upgrade)
			$out .= '<li>'.($wz_functions[$wz_research[$upgrade]['result'][0][1]]['funcs'][0]['amt']+100).'% - <a href="'.$root.'r/'.$upgrade.'">'.$wz_research[$upgrade]['name']."</a></li>\n";
		$out .= "</ul>\n";
	}
	else $out .= "<p><em>No upgrades available.</em></p>\n";
	return $out;
}
function bodyupgrades($cyborg=true, $root='')
{
	global $wz_upgrades, $wz_research, $wz_functions;
	$out = '<h4 id="hpupgrades">HP / kinetic-armor upgrades</h4>';
	$out .= "<ul>";
	foreach ($wz_upgrades[$cyborg?'cyborghp':'vehiclehp']['upgrades'] as $upgrade)
		$out .= '<li>'.($wz_functions[$wz_research[$upgrade]['result'][0][1]]['funcs'][0]['amt']+100).'% / '.($wz_functions[$wz_research[$upgrade]['result'][0][1]]['funcs'][1]['amt']+100).'% - <a href="'.$root.'r/'.$upgrade.'">'.$wz_research[$upgrade]['name']."</a></li>\n";
	$out .= "</ul>\n";
	$out .= '<h4 id="thermalupgrades">Thermal-armor upgrades</h4>';
	$out .= "<ul>";
	foreach ($wz_upgrades[$cyborg?'cyborgthermal':'vehiclethermal']['upgrades'] as $upgrade)
		$out .= '<li>'.($wz_functions[$wz_research[$upgrade]['result'][0][1]]['funcs'][0]['amt']+100).'% - <a href="'.$root.'r/'.$upgrade.'">'.$wz_research[$upgrade]['name']."</a></li>\n";
	$out .= "</ul>\n";
	$out .= '<h4 id="engineupgrades">Engine upgrades</h4>';
	$out .= "<ul>";
	foreach ($wz_upgrades['vehicleengine']['upgrades'] as $upgrade)
		$out .= '<li>'.($wz_functions[$wz_research[$upgrade]['result'][0][1]]['funcs'][0]['amt']+100).'% - <a href="'.$root.'r/'.$upgrade.'">'.$wz_research[$upgrade]['name']."</a></li>\n";
	$out .= "</ul>\n";
	return $out;
}
function structureupgrades($structid, $root='', $base=false)
{
	global $wz_upgrades, $wz_research, $wz_functions;
	if (!is_string($structid)) $structid = $structid['sid'];
	$out = '';
	if ($structid == 'researchfacility' || $structid == 'researchmodule')
	{
		$out .= "<ul>";
		foreach ($wz_upgrades['researchupgrade']['upgrades'] as $upgrade)
			$out .= '<li>'.($wz_functions[$wz_research[$upgrade]['result'][0][1]]['funcs'][0]['amt']+100).'% - <a href="'.$root.'r/'.$upgrade.'">'.$wz_research[$upgrade]['name']."</a></li>\n";
		$out .= "</ul>\n";
	}
	if ($structid == 'factory' || $structid == 'vtolfactory' || $structid == 'cyborgfactory' || $structid == 'factorymodule')
	{
		$out .= "<ul>";
		foreach ($wz_upgrades['factoryupgrade']['upgrades'] as $upgrade)
			$out .= '<li>'.($wz_functions[$wz_research[$upgrade]['result'][0][1]]['funcs'][0]['amt']+100).'% - <a href="'.$root.'r/'.$upgrade.'">'.$wz_research[$upgrade]['name']."</a></li>\n";
		$out .= "</ul>\n";
	}
	if ($structid == 'powergenerator' || $structid == 'powermodule' || $structid == 'oilderrick')
	{
		$out .= "<ul>";
		foreach ($wz_upgrades['powerupgrade']['upgrades'] as $upgrade)
			$out .= '<li>'.($wz_functions[$wz_research[$upgrade]['result'][0][1]]['funcs'][0]['amt']+100).'% - <a href="'.$root.'r/'.$upgrade.'">'.$wz_research[$upgrade]['name']."</a></li>\n";
		$out .= "</ul>\n";
	}
	if ($structid == 'vtolrearmingpad')
	{
		$out .= "<ul>";
		foreach ($wz_upgrades['rearmupgrade']['upgrades'] as $upgrade)
			$out .= '<li>'.($wz_functions[$wz_research[$upgrade]['result'][0][1]]['funcs'][0]['amt']+100).'% - <a href="'.$root.'r/'.$upgrade.'">'.$wz_research[$upgrade]['name']."</a></li>\n";
		$out .= "</ul>\n";
	}
	if ($structid == 'repairfacility')
	{
		$out .= "<ul>";
		foreach ($wz_upgrades['repairupgrade']['upgrades'] as $upgrade)
			$out .= '<li>'.($wz_functions[$wz_research[$upgrade]['result'][0][1]]['funcs'][0]['amt']+100).'% - <a href="'.$root.'r/'.$upgrade.'">'.$wz_research[$upgrade]['name']."</a></li>\n";
		$out .= "</ul>\n";
	}
	$out .= '<h4 id="hpupgrades">HP / armor upgrades</h4>';
	$out .= "<ul>";
	foreach ($wz_upgrades[$base?'structurehp':'wallhp']['upgrades'] as $upgrade)
		$out .= '<li>'.($wz_functions[$wz_research[$upgrade]['result'][0][1]]['funcs'][0]['amt']+100).'% / '.($wz_functions[$wz_research[$upgrade]['result'][0][1]]['funcs'][1]['amt']+100).'% - <a href="'.$root.'r/'.$upgrade.'">'.$wz_research[$upgrade]['name']."</a></li>\n";
	$out .= "</ul>\n";
	return $out;
}

function weapondesc($weapon, $root='')
{
	global $wz_wsubclasses, $wz_research, $wz_wids, $sensors;
	if (is_string($weapon)) $weapon = $GLOBALS['wz_weapons'][$weapon];
	$out = '<p>'.iconimg($weapon['tid']?$weapon['tid']:$weapon['wid'],'w').' <span class="price">$'.$weapon['price'].'</span> <span class="smallgrey indent">Turret ID: '.aval(array_keys($wz_wids,$weapon['wid']),0).'</span></p>';
	if ($weapon['turret'] == 'weapon')
		$out .= '<p>['.weapontype($weapon)."]</p>\n";
	$out .= '<div class="b"><table class="mpad">';
	$out .= '<tr><th class="r nb">&nbsp;</th><th class="l small">Base</th><th class="l small">Upgraded</th></tr>';
	if ($weapon['turret'] == 'weapon')
	{
		$out .= '<tr><th class="r">Damage:</th><td class="l">'.weapondam($weapon).'</td><td class="l">'.weapondam($weapon,$wz_wsubclasses[$weapon['subclass']]['maxdam']).'</td></tr>';
		$out .= '<tr><th class="r">ROF:</th><td class="l">'.weaponrof($weapon).'</td><td class="l">'.weaponrof($weapon,$wz_wsubclasses[$weapon['subclass']]['maxrof']).'</td></tr>';
		if ($weapon['subclass'] != 'emp') $out .= '<tr><th class="r">DPS:</th><td class="l">'.$weapon['dps'].'</td><td class="l">'.round($weapon['dps']*100/(100-$wz_wsubclasses[$weapon['subclass']]['maxrof'])*($wz_wsubclasses[$weapon['subclass']]['maxdam']+100)/100*($wz_wsubclasses[$weapon['subclass']]['maxacc']+100)/100,1).'</td></tr>';
		$out .= '<tr><th class="r">Accuracy:<br /><span class="smallgrey">(close-long)</span></th><td class="l" valign="top"><span class="smallgrey">'.$weapon['sacc'].'-</span>'.$weapon['lacc'].'%</td><td class="l" valign="top"><span class="smallgrey">'.round($weapon['sacc']*($wz_wsubclasses[$weapon['subclass']]['maxacc']+100)/100,1).'-</span>'.round($weapon['lacc']*($wz_wsubclasses[$weapon['subclass']]['maxacc']+100)/100,1).'%</td></tr>';
		$out .= '<tr><th class="r">Range:<br /><span class="smallgrey">(close-long)</span></th><td class="l" colspan="2" valign="top">'."<span class=\"smallgrey\">".round($weapon['srange']/128,1)."-</span><span class=\"fwrap\"> </span>".round($weapon['lrange']/128,1)."</td></tr>";
		if ($weapon['numattackruns']) $out .= '<tr><th class="r">Attack runs:</th><td class="l" colspan="2" valign="top">'.$weapon['numattackruns'].'</td></tr>';
		if ($weapon['subclass'] != 'emp' && $weapon['numattackruns']) $out .= '<tr><th class="r"><abbr title="Damage Per Rearm">DPR</abbr>:</th><td class="l">'.($weapon['damage']*$weapon['lacc']/100*$weapon['nrounds']*$weapon['numattackruns']).'</td><td class="l">'.round($weapon['damage']*$weapon['lacc']/100*$weapon['nrounds']*$weapon['numattackruns']*($wz_wsubclasses[$weapon['subclass']]['maxdam']+100)/100*($wz_wsubclasses[$weapon['subclass']]['maxacc']+100)/100,1).'</td></tr>';
	}
	else if ($weapon['turret'] == 'sensor')
	{
		$out .= '<tr><th class="r">Sensor:</th><td class="l" colspan="2">'.$sensors[$weapon['sensor']].'</td></tr>';
		$out .= '<tr><th class="r">Range:</th><td class="l">'.round($weapon['lrange']/128,1).'</td><td class="l">'.round($weapon['lrange']*($wz_wsubclasses['sensor']['maxrange']+100)/100/128,1).'</td></tr>';
	}
	else
	{
		$out .= '<tr><th class="r">Build points:</th><td class="l">'.$weapon['build'].'</td><td class="l">'.round($weapon['build']*($wz_wsubclasses[$weapon['subclass']]['maxbuild']+100)/100,1).'</td></tr>';
	}
	$out .= '<tr class="nf"><th class="r nb">&nbsp;</th><th class="l small">Base</th><th class="l small">Upgraded</th></tr>';
	$out .= '<tr><th class="r">HP:</th><td class="l" colspan="2" valign="top">'.$weapon['hp']."</td></tr>";
	$out .= '<tr><th class="r">Weight:</th><td class="l" colspan="2" valign="top">'.$weapon['weight']."</td></tr>";
	$out .= "</table></div>\n";
	if ($weapon['prereq']) $out .= "<p><em>Prerequisites:</em> <a href=\"../r/{$weapon['prereq']}\">{$wz_research[$weapon['prereq']]['name']}</a></p>";
	return $out;
}
function templatedesc($template, $root='', $cyborg=true)
{
	global $wz_wsubclasses, $wz_research, $wz_wids, $wz_tids, $sensors, $wz_upgrades, $wz_templates, $wz_bodies;
	if (is_string($template)) $template = $GLOBALS['wz_templates'][$template];
	$weapon = $GLOBALS['wz_weapons'][$template['turrets'][0]];

	if (iconimg($template['tid'],'c'))
	{
		$icon = iconimg($template['tid'],'c');
	}
	else
	{
		$icon = iconimg($template['turrets'][0],'w').iconimg($template['body'],'bp').iconimg($template['propulsion'],'bp');
	}

	$out = '<p>'.$icon.' <span class="price">$'.$template['price'].'</span> <span class="smallgrey indent">Template ID: '.aval(array_keys($wz_tids,$template['tid']),0).'</span></p>';
	if ($weapon['turret'] == 'weapon')
		$out .= '<p>['.weapontype($weapon)."]</p>\n";
	$out .= '<div class="b"><table class="mpad">';
	$out .= '<tr><th class="r nb">&nbsp;</th><th class="l small">Base</th><th class="l small">Upgraded</th></tr>';
	if ($weapon['turret'] == 'weapon')
	{
		$out .= '<tr><th class="r">Damage:</th><td class="l">'.weapondam($weapon).'</td><td class="l">'.weapondam($weapon,$wz_wsubclasses[$weapon['subclass']]['maxdam']).'</td></tr>';
		$out .= '<tr><th class="r">ROF:</th><td class="l">'.weaponrof($weapon).'</td><td class="l">'.weaponrof($weapon,$wz_wsubclasses[$weapon['subclass']]['maxrof']).'</td></tr>';
		if ($weapon['subclass'] != 'emp') $out .= '<tr><th class="r">DPS:</th><td class="l">'.$weapon['dps'].'</td><td class="l">'.round($weapon['dps']*100/(100-$wz_wsubclasses[$weapon['subclass']]['maxrof'])*($wz_wsubclasses[$weapon['subclass']]['maxdam']+100)/100*($wz_wsubclasses[$weapon['subclass']]['maxacc']+100)/100,1).'</td></tr>';
		$out .= '<tr><th class="r">Accuracy:<br /><span class="smallgrey">(close-long)</span></th><td class="l" valign="top"><span class="smallgrey">'.$weapon['sacc'].'-</span>'.$weapon['lacc'].'%</td><td class="l" valign="top"><span class="smallgrey">'.round($weapon['sacc']*($wz_wsubclasses[$weapon['subclass']]['maxacc']+100)/100,1).'-</span>'.round($weapon['lacc']*($wz_wsubclasses[$weapon['subclass']]['maxacc']+100)/100,1).'%</td></tr>';
		$out .= '<tr><th class="r">Range:<br /><span class="smallgrey">(close-long)</span></th><td class="l" colspan="2" valign="top">'."<span class=\"smallgrey\">".round($weapon['srange']/128,1)."-</span><span class=\"fwrap\"> </span>".round($weapon['lrange']/128,1)."</td></tr>";
		if ($weapon['numattackruns']) $out .= '<tr><th class="r">Attack runs:</th><td class="l" colspan="2" valign="top">'.$weapon['numattackruns'].'</td></tr>';
		if ($weapon['subclass'] != 'emp' && $weapon['numattackruns']) $out .= '<tr><th class="r"><abbr title="Damage Per Rearm">DPR</abbr>:</th><td class="l">'.($weapon['damage']*$weapon['lacc']/100*$weapon['nrounds']*$weapon['numattackruns']).'</td><td class="l">'.round($weapon['damage']*$weapon['lacc']/100*$weapon['nrounds']*$weapon['numattackruns']*($wz_wsubclasses[$weapon['subclass']]['maxdam']+100)/100*($wz_wsubclasses[$weapon['subclass']]['maxacc']+100)/100,1).'</td></tr>';
	}
	else if ($weapon['turret'] == 'sensor')
	{
		$out .= '<tr><th class="r">Sensor:</th><td class="l" colspan="2">'.$sensors[$weapon['sensor']].'</td></tr>';
		$out .= '<tr><th class="r">Range:</th><td class="l">'.round($weapon['lrange']/128,1).'</td><td class="l">'.round($weapon['lrange']*($wz_wsubclasses['sensor']['maxrange']+100)/100/128,1).'</td></tr>';
	}
	else
	{
		$out .= '<tr><th class="r">Build points:</th><td class="l">'.$weapon['build'].'</td><td class="l">'.round($weapon['build']*($wz_wsubclasses[$weapon['subclass']]['maxbuild']+100)/100,1).'</td></tr>';
	}
	$out .= '<tr class="nf"><th class="r nb">&nbsp;</th><th class="l small">Base</th><th class="l small">Upgraded</th></tr>';
	$out .= '<tr><th class="r">HP:</th><td class="l" valign="top">'.templatehp($template).'</td><td class="l" valign="top">'.templatehp($template,$cyborg?$wz_upgrades['cyborghp']['max']:$wz_upgrades['vehiclehp']['max'])."</td></tr>";
	$out .= '<tr><th class="r">Armor:</th><td class="l" valign="top">'.bodyarmor($wz_bodies[$template['body']]).'</td><td class="l" valign="top">'.bodyarmor($wz_bodies[$template['body']],$cyborg?$wz_upgrades['cyborgarmor']['max']:$wz_upgrades['vehiclearmor']['max'],$cyborg?$wz_upgrades['cyborgthermal']['max']:$wz_upgrades['vehiclethermal']['max'])."</td></tr>";
	$out .= '<tr><th class="r">Speed:</th><td class="l" valign="top">'.templatespeed($template).'</td><td class="l" valign="top">'.templatespeed($template,$wz_upgrades['vehicleengine']['max'])."</td></tr>";
	$out .= '<tr><th class="r">Weight:</th><td class="l" colspan="2" valign="top">'.$template['weight']."</td></tr>";
	$out .= "</table></div>\n";
	if ($template['prereq']) $out .= "<p><em>Prerequisites:</em> <a href=\"../r/{$template['prereq']}\">{$wz_research[$template['prereq']]['name']}</a></p>";
	return $out;
}
function structuredesc($struct, $root='', $base=false)
{
	global $wz_wsubclasses, $wz_research, $wz_wids, $wz_sids, $sensors, $wz_upgrades;
	if (is_string($struct)) $struct = $GLOBALS['wz_structures'][$struct];
	$weapon = array();
	if ($struct['turrets'])
	{
		$weapon = $GLOBALS['wz_weapons'][$struct['turrets'][0]];
	}

	$out = '<p>'.iconimg($struct['sid'],'c').' <span class="price">$'.$struct['price'].'</span> <span class="smallgrey indent">Structure ID: '.aval(array_keys($wz_sids,$struct['sid']),0).'</span></p>';
	if ($weapon && $weapon['turret'] == 'weapon')
		$out .= '<p>['.weapontype($weapon)."]</p>\n";
	$out .= '<div class="b"><table class="mpad">';
	$out .= '<tr><th class="r nb">&nbsp;</th><th class="l small">Base</th><th class="l small">Upgraded</th></tr>';
	if ($weapon['turret'] == 'weapon')
	{
		$out .= '<tr><th class="r">Damage:</th><td class="l">'.weapondam($weapon).'</td><td class="l">'.weapondam($weapon,$wz_wsubclasses[$weapon['subclass']]['maxdam']).'</td></tr>';
		$out .= '<tr><th class="r">ROF:</th><td class="l">'.weaponrof($weapon).'</td><td class="l">'.weaponrof($weapon,$wz_wsubclasses[$weapon['subclass']]['maxrof']).'</td></tr>';
		if ($weapon['subclass'] != 'emp') $out .= '<tr><th class="r">DPS:</th><td class="l">'.$weapon['dps'].'</td><td class="l">'.round($weapon['dps']*100/(100-$wz_wsubclasses[$weapon['subclass']]['maxrof'])*($wz_wsubclasses[$weapon['subclass']]['maxdam']+100)/100*($wz_wsubclasses[$weapon['subclass']]['maxacc']+100)/100,1).'</td></tr>';
		$out .= '<tr><th class="r">Accuracy:<br /><span class="smallgrey">(close-long)</span></th><td class="l" valign="top"><span class="smallgrey">'.$weapon['sacc'].'-</span>'.$weapon['lacc'].'%</td><td class="l" valign="top"><span class="smallgrey">'.round($weapon['sacc']*($wz_wsubclasses[$weapon['subclass']]['maxacc']+100)/100,1).'-</span>'.round($weapon['lacc']*($wz_wsubclasses[$weapon['subclass']]['maxacc']+100)/100,1).'%</td></tr>';
		$out .= '<tr><th class="r">Range:<br /><span class="smallgrey">(close-long)</span></th><td class="l" colspan="2" valign="top">'."<span class=\"smallgrey\">".round($weapon['srange']/128,1)."-</span><span class=\"fwrap\"> </span>".round($weapon['lrange']/128,1)."</td></tr>";
		if ($weapon['numattackruns']) $out .= '<tr><th class="r">Attack runs:</th><td class="l" colspan="2" valign="top">'.$weapon['numattackruns'].'</td></tr>';
		if ($weapon['subclass'] != 'emp' && $weapon['numattackruns']) $out .= '<tr><th class="r"><abbr title="Damage Per Rearm">DPR</abbr>:</th><td class="l">'.($weapon['damage']*$weapon['lacc']/100*$weapon['nrounds']*$weapon['numattackruns']).'</td><td class="l">'.round($weapon['damage']*$weapon['lacc']/100*$weapon['nrounds']*$weapon['numattackruns']*($wz_wsubclasses[$weapon['subclass']]['maxdam']+100)/100*($wz_wsubclasses[$weapon['subclass']]['maxacc']+100)/100,1).'</td></tr>';
		$out .= '<tr class="nf"><th class="r nb">&nbsp;</th><th class="l small">Base</th><th class="l small">Upgraded</th></tr>';
	}
	else if ($weapon['turret'] == 'sensor')
	{
		$out .= '<tr><th class="r">Sensor:</th><td class="l" colspan="2">'.$sensors[$weapon['sensor']].'</td></tr>';
		$out .= '<tr><th class="r">Range:</th><td class="l">'.round($weapon['lrange']/128,1).'</td><td class="l">'.round($weapon['lrange']*($wz_wsubclasses['sensor']['maxrange']+100)/100/128,1).'</td></tr>';
		$out .= '<tr class="nf"><th class="r nb">&nbsp;</th><th class="l small">Base</th><th class="l small">Upgraded</th></tr>';
	}
	else if ($weapon)
	{
		$out .= '<tr><th class="r">Build points:</th><td class="l">'.$weapon['build'].'</td><td class="l">'.round($weapon['build']*($wz_wsubclasses[$weapon['subclass']]['maxbuild']+100)/100,1).'</td></tr>';
		$out .= '<tr class="nf"><th class="r nb">&nbsp;</th><th class="l small">Base</th><th class="l small">Upgraded</th></tr>';
	}
	$out .= '<tr><th class="r">HP:</th><td class="l" valign="top">'.structurehp($struct).'</td><td class="l" valign="top">'.structurehp($struct,$base?$wz_upgrades['structurehp']['max']:$wz_upgrades['wallhp']['max'])."</td></tr>";
	$out .= '<tr><th class="r">Armor:</th><td class="l" valign="top">'.structurearmor($struct).'</td><td class="l" valign="top">'.structurearmor($struct,$base?$wz_upgrades['structurearmor']['max']:$wz_upgrades['wallarmor']['max'])."</td></tr>";
	$out .= '<tr><th class="r">Strength:</th><td class="l" valign="top" colspan="2">'.($GLOBALS['strengths'][$struct['strength']])."</td></tr>";
	$out .= '<tr><th class="r">Size:</th><td class="l" valign="top" colspan="2">'.$struct['w'].'&times;'.$struct['h']."</td></tr>";
	$out .= '<tr><th class="r">Height:</th><td class="l" valign="top" colspan="2">'.$struct['z']."</td></tr>";
	$out .= "</table></div>\n";
	if ($struct['prereq']) $out .= "<p><em>Prerequisites:</em> <a href=\"../r/{$struct['prereq']}\">{$wz_research[$struct['prereq']]['name']}</a></p>";
	if ($base) $out .= guide('b/'.$struct['sid']);
	return $out;
}

function bodydesc($body, $root='', $cyborg=false)
{
	global $wz_research, $wz_upgrades, $wz_bids;
	if (is_string($body)) $body = $GLOBALS['wz_bodies'][$body];

	$out = '<p>'.iconimg($body['bid'],'bp').' <span class="price">$'.$body['price'].'</span> <span class="smallgrey indent">Body ID: '.aval(array_keys($wz_bids,$body['bid']),0).'</span></p>';
	$out .= '<div class="b"><table class="mpad">';
	$out .= '<tr><th class="r nb">&nbsp;</th><th class="l small">Base</th><th class="l small">Upgraded</th></tr>';
	$out .= '<tr><th class="r">HP:</th><td class="l" valign="top">'.$body['hp'].'</td><td class="l" valign="top">'.iupg($body['hp'],$cyborg?$wz_upgrades['cyborghp']['max']:$wz_upgrades['vehiclehp']['max'])."</td></tr>";
	$out .= '<tr><th class="r">Armor:</th><td class="l" valign="top">'.bodyarmor($body).'</td><td class="l" valign="top">'.bodyarmor($body,$cyborg?$wz_upgrades['cyborgarmor']['max']:$wz_upgrades['vehiclearmor']['max'],$cyborg?$wz_upgrades['cyborgthermal']['max']:$wz_upgrades['vehiclethermal']['max'])."</td></tr>";
	$out .= '<tr><th class="r">Engine:</th><td class="l" valign="top">'.$body['engine'].'</td><td class="l" valign="top">'.iupg($body['engine'],$wz_upgrades['vehicleengine']['max'])."</td></tr>";
	$out .= '<tr><th class="r">Weight:</th><td class="l" colspan="2" valign="top">'.$body['weight']."</td></tr>";
	$out .= "</table></div>\n";
	if ($body['prereq']) $out .= "<p><em>Prerequisites:</em> <a href=\"../r/{$body['prereq']}\">{$wz_research[$body['prereq']]['name']}</a></p>";
	return $out;
}

function showprereqs($id, &$ar, $root='', $last=false)
{
	global $wz_research, $wz_structures;
	if (in_array($id,$ar))
		return "<li".($last?' class="last"':'')."><span class=\"expcol nec\"></span><div class=\"smallgrey ti\">{$wz_research[$id]['name']} <em>(See above)</em></div></li>\n";
	if (substr($id,0,5)=='../s/')
		return "<li".($last?' class="last"':'')."><span class=\"expcol nec\"></span><div class=\"ti\"><em>".$wz_structures[substr($id,5)]['name']." built</em></div></li>\n";
	$ar[] = $id;
	$out = '<li'.($last?' class="last"':'').'>';
	$out .= ($id==$GLOBALS['id']?('<div class="ti">'.$wz_research[$id]['name']):((($wz_research[$id]['prereqs'])&&(substr($wz_research[$wz_research[$id]['prereqs'][0]]['name'],0,4) !== 'CAM1' && $wz_research[$id]['prereqs'][0] != $id)?"<a href=\"#\" id=\"btn$id\" onclick=\"return toggle('$id',this)\" class=\"expcol col\"></a>":($id!=$GLOBALS['id']?'<span class="expcol nec"></span>':'')).'<div class="ti"><a href="'.$root.'r/'.$id.'">'.$wz_research[$id]['name'].'</a>'));
	if ($wz_research[$id]['prereqs'])
	{
		if (substr($wz_research[$wz_research[$id]['prereqs'][0]]['name'],0,4) === 'CAM1')
			$out .= ' <span class="grey">[<abbr title="This does not have any prerequisites, and can be researched at the beginning of any game.">base</abbr>]</span></div>';
		else if ($wz_research[$id]['prereqs'][0] == $id)
			$out .= ' <span class="grey">[<abbr title="This does not need to be researched; it has already been researched in every game.">automatic</abbr>]</span></div>';
		else
		{
			$out .= "</div>\n<ul id=\"prereq$id\">\n";
			$n = count($wz_research[$id]['prereqs']);
			//if ($wz_research[$id]['prereqs'][$n-1] == 'synapticlink') $n--;
			foreach ($wz_research[$id]['prereqs'] as $i => $prereq)
				$out .= showprereqs($prereq, $ar, $root, $i==($n-1));
			$out .= "</ul>\n";
		}
	} else $out .= '</div>';
	return $out."</li>\n";
}
function gentechtree($id, &$ar, $root='', $last=false, $depth=0, $parent='')
{
	global $wz_research, $wz_structures, $deepest, $deepests;
	if ($depth>$deepest)
	{
		$deepest = $depth;
		$deepests = array($id);
	}
	else if ($depth == $deepest) $deepests[] = $id;
	if (in_array($id,$ar))
		return "<li".($last?' class="last"':'')."><span class=\"expcol nec\"></span><div class=\"smallgrey ti\"><a href=\"{$root}r/{$id}\">{$wz_research[$id]['name']}</a> <em><a href=\"#{$id}\" onclick=\"highlight('{$id}'); return expandto('{$id}')\">(See above)</a></em></div></li>\n";
	$ar[] = $id;
	$out = '<li'.($last?' class="last"':'').' id="li'.$id.'">';
	$out .= (in_array($id,array('machinegun','sensorturret','engineering'))?('<div class="ti" id="'.$id.'"><a href="'.$root.'r/'.$id.'">'.$wz_research[$id]['name'].'</a>'):((($wz_research[$id]['allows'])&&(substr($wz_research[$wz_research[$id]['allows'][0]]['name'],0,4) !== 'CAM1' && $wz_research[$id]['allows'][0] != $id)?"<a href=\"#\" id=\"btn$id\" onclick=\"return toggle('$id',this)\" class=\"expcol col\"></a>":($id!=$GLOBALS['id']?'<span class="expcol nec"></span>':'')).'<div class="ti" id="'.$id.'"><a href="'.$root.'r/'.$id.'" id="item'.$id.'">'.$wz_research[$id]['name'].'</a>'));
	if ($wz_research[$id]['allows'])
	{
		$out .= "</div>\n<ul id=\"prereq$id\">\n";
		$n = count($wz_research[$id]['allows']);
		foreach ($wz_research[$id]['allows'] as $i => $allow)
			$out .= gentechtree($allow, $ar, $root, $i==($n-1), $depth+1, $id);
		$out .= "</ul>\n";
	} else $out .= '</div>';
	return $out."</li>\n";
}
function showallows($id,$root='')
{
	global $wz_research, $wz_structures;
	$out = "<div class=\"b\"><ul>\n";
	if (!$wz_research[$id]['allows'])
		return '<p><em>'.$wz_research[$id]['name'].' is not required for any research.</em></p>';
	else foreach ($wz_research[$id]['allows'] as $allow)
	{
		if ($allow == $id) continue;
		$prereqs = $wz_research[$allow]['prereqs'];
		$fprereqs = array($wz_research[$id]['name']);
		foreach ($prereqs as $prereq) if ($prereq != $id)
			$fprereqs[] = (substr($prereq,0,5)=='../s/'?'<em>'.$wz_structures[substr($prereq,5)]['name'].' built</em>':'<a href="'.$root.'r/'.$prereq.'">'.$wz_research[$prereq]['name'].'</a>');
		$out .= '<li><a href="'.$root.'r/'.$allow.'"><strong>'.$wz_research[$allow]['name'].'</strong></a> &laquo; <span class="small">'.implode(' + ',$fprereqs)."</span></li>\n";
	}
	return $out.'</ul></div>';
}
function aval($array, $i)
{
	return $array[$i];
}
function showdesc($id, $root='')
{
	if (!$id) return 'Error: No ID provided.';
	if ($id == 'NOID') $id = '';
	global $wz_research, $wz_weapons, $wz_templates, $wz_functions, $wz_bodies, $wz_propulsions, $wz_structures, $upgrades, $dt, $wz_rids, $subclasses;
	$out = '<p>'.iconimg($id,'r').'<span class="price">$'.min(intval($wz_research[$id]['price']/32),450).'</span> <span class="smallgrey indent">'.aval(array_keys($wz_rids,$id),0).'</span></p>';
	$out .= '<p>'.($wz_research[$id]['major']?'<span class="small">[<strong>Major research</strong>]</span>':'<span class="small">[Minor research]</span>').'</p>';
	if ($wz_research[$id]['desc']) $out .= '<blockquote><em>'.implode('<br />',$wz_research[$id]['desc']).'</em></blockquote>';
	if (count($wz_research[$id]['result']))
	{
		$out .= "<ul class=\"b\">\n";
		foreach ($wz_research[$id]['result'] as $result)
		{
			if ($result[0]=='function')
			{
				$funcs = $wz_functions[$result[1]];
				foreach ($funcs['funcs'] as $func)
				{
					if ($func['subclass'] && $func['type']=='weaponrof')
						$out .= '<li>Upgrades '.$subclasses[$func['subclass']].' rate of fire to '.round(10000/(100-$func['amt']),0).'%</li>';
					else if ($func['subclass'])
						$out .= '<li>Upgrades '.$subclasses[$func['subclass']].' '.$dt[$func['type']].' to '.($func['amt']+100).'%</li>';
					else if (isset($upgrades[$func['type']]))
						$out .= '<li>Upgrades '.$upgrades[$func['type']].' to '.($func['amt']+100).'%</li>';
					else
						$out .= '<li>Unknown upgrade: '.$func['type'].'</li>';
				}
			}
			else if ($result[0]=='weapon' && $result[1]=='autorepair')
				$out .= '<li>Enables auto-repair</li>';
			else if ($result[0]=='weapon')
				$out .= '<li>Gives new turret: <a href="'.$root.'w/'.$result[1].'">'.$wz_weapons[$result[1]]['name'].'</a></li>';
			else if ($result[0]=='cyborg')
				$out .= '<li>Gives new cyborg: <a href="'.$root.'c/'.$result[1].'">'.$wz_templates[$result[1]]['name'].'</a></li>';
			else if ($result[0]=='rplcweapon')
				$out .= '<li>Automatically replaces: <a href="'.$root.'w/'.$result[1].'">'.$wz_weapons[$result[1]]['name'].'</a></li>';
			else if ($result[0]=='redweapon')
				$out .= '<li>Makes turret obsolete: <a href="'.$root.'w/'.$result[1].'">'.$wz_weapons[$result[1]]['name'].'</a></li>';
			else if ($result[0]=='redstructure')
				$out .= '<li>Makes structure obsolete: '.$wz_structures[$result[1]]['name'].'</li>';
			else if ($result[0]=='struct')
				$out .= '<li>Gives new structure: '.$wz_structures[$result[1]]['name'].'</li>';
			else if ($result[0]=='body')
				$out .= '<li>Gives new body: '.$wz_bodies[$result[1]]['name'].'</li>';
			else if ($result[0]=='propulsion')
				$out .= '<li>Gives new propulsion: '.$wz_propulsions[$result[1]]['name'].'</li>';
			else
				$out .= '<li>Unknown result: '.$result[0].'</li>';
		}
		$out .= "</ul>\n";
	}
	if ($wz_research[$id]['prereqs'][0] == $id)
		$out .= '<p class="small"><em>Automatic</em> &ndash; This does not need to be researched; it has already been researched in every game.</p>';
	if (substr($wz_research[$wz_research[$id]['prereqs'][0]]['name'],0,4) === 'CAM1')
		$out .= '<p class="small"><em>Base</em> &ndash; This does not have any prerequisites, and can be researched at the beginning of any game.</p>';
	return $out;
}

/*
 * COMMENTARY FUNCTIONS
 */ 

$forgotten = false;
$falsecookie = false;
if (!$_POST['forget'] && $_POST['remember'] && ($_COOKIE['name'] != trim($_POST['name']) && ($isauth || simplify($_POST['captcha'])=='mg' || simplify($_POST['captcha'])=='machinegun')))
{
	setcookie('name', trim($_POST['name']).($isadmin?':'.$adminpass:':mg'), time()+365*24*60*60,'/');
	$_COOKIE['name'] = trim($_POST['name']);
	$falsecookie = true;
}
else if ($_POST['forget'] && $_COOKIE['name'])
{
	setcookie('name', '', time()-60*60,'/');
	$forgotten = true;
	$_COOKIE['name'] = '';
	$_POST['name'] = '';
	$_REQUEST['name'] = '';
	$isauth = $isadmin = false;
}

function onreldate($time)
{
	if (!$time) return 'never';
	$suf = 'ago';
	$rtime = ($ctime=time()) - $time;
	if (!$rtime) return 'just now';
	if ($rtime < 0) { $suf = 'from now'; $rtime = -$rtime; }
	if ($rtime < 60) return $rtime.' second'.($rtime==1?'':'s').' '.$suf;
	$rtime = intval($rtime/60);
	if ($rtime < 60) return $rtime.' minute'.($rtime==1?'':'s').' '.$suf;
	if ($rtime < 120) return 'an hour and '.($rtime-60).' minute'.($rtime==61?'':'s').' '.$suf;
	//$rtime = intval($rtime/60);
	//echo '[rtime '.$rtime.']';
	if (intval($time/86400)==intval($ctime/86400)) // same day
		return intval($rtime/60).($rtime%60>=30?' and a half':'').' hour'.($rtime<90?'':'s').' '.$suf;
	if (($daysago=intval($ctime/86400)-intval($time/86400))==1)
		return 'yesterday';
	if ($daysago==-1)
		return 'tomorrow';
	if ($daysago<=3)
		return $daysago.' days ago';
	$ctimea=getdate($ctime);$timea=getdate($time);
	if ($ctimea['year']==$timea['year'])
		return 'on '.date('M j',$time);
	return 'on '.date('M j, Y',$time);
}

function comments($id, $root='')
{
	global $_PERSIST, $isauth, $isadmin, $falsecookie, $forgotten;
	$posted = false;
	if ($id[1] == '/')
		$comments = &$_PERSIST[$id[0]]['subdirs'][substr($id,2)];
	else
		$comments = &$_PERSIST[$id];
	echo '<h3 style="margin-top:100px" id="comments">Commentary</h3>';
	if ($_POST['msg'])
	{
		if (!($_POST['name']=trim($_POST['name'])) || strlen($_POST['name'])>64)
			echo '<p style="color:red">Error: Name is invalid.</p>';
		if (!$isauth && simplify($_REQUEST['captcha'])!='machinegun' && simplify($_REQUEST['captcha'])!='mg')
			echo '<p style="color:red">Error: Wrong answer.</p>';
		else
		{
			if ($isadmin)
			{
				$comments[] = array(
					'name' => stripslashes($_POST['name']),
					'msg' => stripslashes($_POST['msg']),
					'time' => time(),
					'admin' => true
				);
			}
			else
			{
				$comments[] = array(
					'name' => stripslashes($_POST['name']),
					'msg' => stripslashes($_POST['msg']),
					'time' => time()
				);
			}
			if (persist_update())
				$posted = true;
			else
				echo '<p style="color:red">Error: Internal server error. Please try again later.</p>';
		}
	}
	else if ($_POST['remember'] && !$isauth && simplify($_REQUEST['captcha'])!='machinegun' && simplify($_REQUEST['captcha'])!='mg')
	{
		echo '<p class="msg" style="color:red">Error: Wrong answer.</p>';
	}
	if ($comments)
	{
		foreach ($comments as $i => $comment)
		{
			if (count($comments)>10 && $i == 0)
				echo '<div id="morecomments">';
			if (count($comments)>10 && $i == count($comments)-10)
			{
?>
</div>
<script type="text/javascript">
<!--
document.getElementById('morecomments').style.display = 'none';
document.write('<p class="backforward" id="morecommentslink" style="text-align: left;"><a href="#comments" onclick="return morecomments();">Earlier comments</a></p>');
function morecomments()
{
	document.getElementById('morecomments').style.display = 'block';
	document.getElementById('morecommentslink').style.display = 'none';
	return false;
}
-->
</script>
<?php
			}
			echo '<p><em><strong>'.htmlspecialchars($comment['name']).'</strong>'.($comment['admin']?' <span style="color:#556688;font-size:10pt;">[Admin]</span>':'').' wrote '.onreldate($comment['time']).':</em></p><blockquote><p>'.($comment['htmlcomment']?$comment['msg']:nl2br(htmlspecialchars($comment['msg']))).'</p></blockquote>';
		}
	}
	else echo '<p><em>No comments yet.</em></p>';
	echo '<h4>Add a comment</h4>';
	if ($posted) echo '<p class="msg">Success!</p>';
	echo '<form action="'.$root.(tourl($id)=='index'?'./':tourl($id)).'#comments" method="post" onsubmit="if (!document.getElementById(\'captcha\').value) {alert(\'Please answer the question. (Hint: The answer is machinegun)\');return false;}"><p id="testparagraph">Name:<br /><input type="text" class="textbox" name="name" value="'.($_POST['name']?$_POST['name']:$_COOKIE['name']).'" size="20" /> <input type="checkbox" name="remember" id="remember" value="remember"'.($_COOKIE['name']?' checked="checked" />':' />').'<label for="remember">Remember name</label><br />Message:<br /><textarea class="textbox" id="msg" name="msg" rows="8" cols="40">'.($posted?'':$_REQUEST['msg']).'</textarea><br />'.($isauth&&$_COOKIE['name']&&!$falsecookie||$isadmin?'':'To prove you are not a spammer, you must correctly answer this question:<br />The first weapon you get in the single-player campaign is?<br /><input class="textbox" type="text" name="captcha" id="captcha" value="'.($isauth?'machinegun':$_REQUEST['captcha']).'" /><br />(<em>Hint: The answer is machinegun</em>)<br />').'<br /><input type="submit" value="Submit" /></p></form>';
?>
<script>
<!--
document.getElementById('msg').style.width = ''+(document.getElementById('testparagraph').offsetWidth-10>650?650:document.getElementById('testparagraph').offsetWidth-10)+'px';
if (document.getElementById('testparagraph').offsetWidth-10>650) document.getElementById('msg').style.height = '200px';
-->
</script>
<?php
	if ($forgotten) echo '<p class="msg">Your name has been forgotten.</p>';
	if ($_COOKIE['name']) echo '<form action="'.$root.(tourl($id)=='index'?'./':tourl($id)).'#comments" method="post"><input type="submit" class="small" name="forget" value="Forget name" /></form>';
}

function guide($id, $gen = false, $editbtn = true)
{
	global $guide, $isadmin;
	$root = '../';
	if ($id[1] == '/')
		$msg = &$guide[$id[0]]['subdirs'][substr($id,2)];
	else
	{
		$msg = &$guide[$id];
		$root = '';
	}
	$out = '';
	if (isset($msg)) $out = $msg['text'];
	else if ($gen && $gen !== 'php')
	{
		$msg['gen'] = $gen;
		persist_update('guide');
	}
	if ($editbtn)
	{
		if ($gen === 'php')
			$out = '<?php if ($isadmin) echo \'<p class="editbtn"><a href="'.$root.'editguide.php?p='.$id.'" target="_blank">Edit</a></p>\'; ?>'.($id=='index'||$id=='panel'?'<div style="clear:right;height:0;"></div>':'').$out;
		else if ($id == 'index' && $isadmin)
			$out = '<p class="editbtn"><a href="'.$root.'editguide.php?p='.$id.'" target="_blank">Edit</a></p><div style="clear:right;height:0;"></div>'.$out;
		else if ($msg['gen'])
			$out = '<script type="text/javascript"><!--
if (isadmin()) document.write(\'<p class="editbtn"><a href="'.$root.'editguide.php?p='.$id.'" target="_blank">Edit</a></p>\');
--></script>'.$out;
		else if ($_COOKIE['name'] && $isadmin)
			$out = '<p class="editbtn"><a href="'.$root.'editguide.php?p='.$id.'" target="_blank">Edit</a></p>'.$out;
	}
	return $out;
}
function getguide($id)
{
	echo '<'.'?php echo guide(\''.$id.'\'); ?'.'>';
}
function leftnav($id)
{
	global $guide;
	if ($id[1] == '/')
		$msg = &$guide[$id[0]]['subdirs'][substr($id,2)];
	else
		$msg = &$guide[$id];
	if ($msg['cachedsb']) return $msg['cachedsb'];
	else
	{
		$msg['cachedsb'] = genleftnav($msg['text']);
		persist_update('guide');
	}
	return $msg['cachedsb'];
}
function genleftnav($text)
{
	preg_match_all('/<h(3|4)[^>]* id="([a-z0-9]+)">([^<]*)<\/h(3|4)>/', $text, $matches, PREG_SET_ORDER);
	$depth = 0;
	$ln = '';
	//print_r($matches);
	foreach ($matches as $match)
	{
		if ($match[1]=='3' && $depth==2)
		{
			$ln .= '</ul>';
			$depth = 1;
		}
		if ($match[1]=='3')
		{
			if ($depth) $ln .= '</li>'; $depth = 1;
			$ln .= '<li><a href="#'.$match[2].'">'.$match[3].'</a>';
		}
		else
		{
			if ($depth <= 1) $ln .= '<ul>'; $depth = 2;
			$ln .= '<li><a href="#'.$match[2].'">'.$match[3].'</a></li>';
		}
	}
	return $ln;
}

function editguide($id, $edit, $gen=false)
{
	global $guide;
	$success = true;
	if ($id[1] == '/')
		$msg = &$guide[$id[0]]['subdirs'][substr($id,2)];
	else
		$msg = &$guide[$id];
	if (!isset($msg) && $gen) $msg['gen'] = $gen;
	$msg['text'] = $edit;
	unset($msg['cachedsb']);
	persist_update('guide') || $success = false;
	if ($step = $msg['gen']) include 'generate.php';
	return $success;
}

//print_r($_REQUEST); echo $isadmin.$isauth;

?>