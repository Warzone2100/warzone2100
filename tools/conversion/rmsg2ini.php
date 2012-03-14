#!/usr/bin/env php
<?php
/***********************************************************************************
* Warzone 2100 RMSG to INI converter
* Copyright (C) 2012 Michal Dutkiewicz aka Emdek <emdeck@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*
***********************************************************************************/

function convert_rmsg($input, &$errors)
{
	$output = '';
	$data = explode("\n", $input);

	for ($i = 0, $c = count($data); $i < $c; ++$i)
	{
		$line = trim($data[$i], "\t\r\n ");

		if (strlen($line) < 5)
		{
			continue;
		}

		if ($line[0] == '.')
		{
			if (substr($line, -5) != 'NULL,')
			{
				$output.= ((substr($output, -2) == '\\'."\n")?"\n":'').substr($line, 1, -1)."\n";
			}
		}
		else if ($line[0] == '_')
		{
			$output.= $line.'\\'."\n";
		}
		else if (preg_match('#^[A-Z][a-zA-Z0-9_]+$#', $line))
		{
			$output.= ($output?"\n":'').'['.$line.']'."\n".'text = \\'."\n";
		}
		else
		{
			$errors[] = ($i + 1);
		}
	}

	return $output;
}

if (count($argv) == 1)
{
	echo 'Usage:
'.(is_executable($argv[0])?'':'php ').$argv[0].' /path/to/file.rmsg [/path/to/log/file.txt]
';

	die();
}

if (!file_exists($argv[1]))
{
	echo 'Can not find specified path:
'.$argv[1];

	die();
}

$log = 'Converting data from path:
'.$argv[1].'

';

if (is_dir($argv[1]))
{
	$argv[1] = rtrim($argv[1], '\\/');

	echo 'Converting all files with .rmsg extension from specified directory by creating new with .ini suffix.
';

	$files = glob($argv[1].'/*.rmsg');
	$total = 0;
	$skipped = 0;

	for ($i = 0, $c = count($files); $i < $c; ++$i)
	{
		$log.= $files[$i].'
';
		$errors = array();
		$path = substr($files[$i], 0, -4).'ini';

		if (file_exists($path) && (!isset($argv[3]) || $argv[3] != '!'))
		{
			$log.= 'Skipped due to already existing file with the same path!
'.$path.'
';

			++$skipped;
		}
		else if (!file_put_contents($path, convert_rmsg(file_get_contents($files[$i]), $errors)))
		{
			$log.= 'Saving output file failed!
'.$path.'
';

			++$skipped;
		}

		$total += count($errors);

		$log.= ($errors?count($errors):'No').' skipped lines (total: '.$total.')
'.($errors?implode(', ', $errors)."\n":'').'
';
	}

	$log.= 'Total: '.($total?$total:'No').' skipped lines'.($skipped?"\n".$skipped.' files skipped!':'');
}
else
{
	$errors = array();
	$log.= 'Single file

';

	echo convert_rmsg(file_get_contents($argv[1]), $errors);

	$log.= ($errors?count($errors):'No').' skiped lines
'.($errors?implode(', ', $errors)."\n":'');
}

if (isset($argv[2]))
{
	$log.= "\n";

	if ($argv[2] == '!')
	{
		echo $log;
	}
	else
	{
		file_put_contents($argv[2], $log);
	}
}
?>
