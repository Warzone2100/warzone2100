#!/usr/bin/env python

""" Splits and scales tertile texpages into mipmaps.

Finds and filters files based on the 'extension' configuration setting, and
of those, groups where available to allow mipmap output to be created from
as few as one input texpage and as many texpages as there are output
resolutions.

Filenames are grouped together if, after all cased letters are lowered and
file extensions are stripped, they have identical names. if, after the
dotted is stripped they still have at least one period in the filename, and
only numeric digits proceed that period, then all text up to, but not
including the last remaining period is used in the comparison. Thus, the
following filenames are grouped together to represent different resolutions
of the same conceptual texpage:

tertilec1hw.tga
tertilec1hw.53.pcx
tertilec1hw.128.pcx

However, 'tertilec1hw.a23.pcx' will create a new 'tertilec1hw.a23' group.

Files are handled on a first-come basis, if two files that are considered
to be part of the same group also have the same resolution, the first one
found will be used, and all subsequent ones with the same resolution in
that group will be discarded. Since this behavior is unpredictable and
operating-system-dependent, it should not be relied upon, and the user
should take care to have have no more than one texpage of each resolution
per group.

"""

__version__ = "1.2"
__author__ = "Kevin Gillette"

# --------------------------------------------------------------------------
# texpage_convert.py v1.2 by Kevin Gillette (kage)
# --------------------------------------------------------------------------
# ***** BEGIN GPL LICENSE BLOCK *****
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# ***** END GPL LICENCE BLOCK *****
# --------------------------------------------------------------------------

import os, sys, shutil
from subprocess import Popen, PIPE

def ds_ppm_parser(file):
	tokens = list()
	gen = iter(file)
	for line in gen:
		comment = line.find('#')
		if comment != -1:
			line = line[:comment]
		tokens.extend(line.split())
		if len(tokens) >= 4: break
	else:
		raise RuntimeError('Invalid PPM file')
	header = [tokens[0]] + map(int, tokens[1:4])
	yield header
	del tokens[:4]
	magicnum, w, h, maxval = header
	magicnum = magicnum.lower()
	if magicnum == "p3":
		while True:
			while len(tokens) >= 3:
				yield map(int, tokens[:3])
				del tokens[:3]
			try:
				tokens.extend(gen.next().split())
			except StopIteration:
				break
	elif magicnum == "p6":
		if len(tokens) < 1:
			tokens = list(gen)
		if maxval < 256: chunksize = 1
		else: chunksize = 2
		raster, tokens = ''.join(tokens), list()
		for i in xrange(0, len(raster), chunksize):
			num = ord(raster[i])
			if chunksize > 1: num = (num << 8) + ord(raster[i + 1])
			tokens.append(num)
			if len(tokens) >= 3:
				yield tokens[:3]
				del tokens[:3]
	else:
		raise RuntimeError('Parser can only handle P3 or P6 files')

def dump_to_radar(gen, outfile):
	gen.next()
	for rgb in gen:
		rgb = [hex(int(c)).split('x')[-1].zfill(2) for c in rgb]
		outfile.write(''.join(rgb) + '\n')

def handle_conf(iterable, config):
	""" Simple configuration loader.

	When provided with an iterable datatype, be it a file object (from
	which it will extract one directive per line), or arguments from the
	command line (each distinct "word" will represent a full directive),
	each token will	either be ignored (blank lines and comments), unset a
	directive (nothing after the equal-sign), or set a directive.
	Unsetting a directive will cause scripted defaults to be used, and not
	setting a given directive will have the same effect.

	"""

	for text in iterable:
		text = text.lstrip().rstrip('\n')
		if text.startswith('#'): continue
		parts = text.split('=', 1)
		if len(parts) == 2:
			parts[0] = parts[0].rstrip()
			if parts[1]:
				print 'setting "%s" to "%s"' % tuple(parts)
				config[parts[0]] = parts[1]
			else:
				print 'defaulting "%s"' % parts[0]
				del config[parts[0]]

def makedir(exportpath, dirname):
	""" Given a basename, create a directory in exportpath

	If the directory already exists, remove all png's contained therein.
	If it	exists but is not a directory, rename the file to something
	similar but unused. If no exception is raised, then a directory by
	the specified name will have been created and its absolute path will
	have been returned as a string.

	"""

	dirpath = os.path.join(exportpath, dirname)
	if os.path.exists(dirpath):
		if os.path.isdir(dirpath):
			print dirname, "already exists: removing contained png files"
			for fn in os.listdir(dirpath):
				if fn.lower().endswith('.png'): os.remove(os.path.join(dirpath, fn))
			return dirpath
		else:
			count = 0
			while os.path.exists("%s.%i" % (dirpath, count)):
				count += 1
			new_path = "%s.%i" % (dirpath, count)
			shutil.move(dirpath, new_path)
			print "error:", dirname, "is not a directory. moving file to", new_path
	print "creating directory:", dirname
	os.mkdir(dirpath)
	return dirpath

def nearest_resolution(initial_index, arr):
	""" Find the best resolution from which to scale.

	initial_index - the position in the global variable 'resolutions' of
	                the desired output resolution.

	arr - same length as 'resolutions' and contains only True and False
	      values. True represents a full-quality texpage for use in scaling,
	      while False represents resolutions for which scaling will be
	      needed to generate.

	Resolutions greater than the desired one are always favored above
	smaller resolutions, with priority being given to resolutions closer
	to the desired output resolution.

	"""

	for i in range(initial_index + 1, len(arr)):
		if arr[i]: return (i, True)
	seq = range(0, initial_index)
	seq.reverse()
	for i in seq:
		if arr[i]: return (i, False)
	assert False, "should always have at least one input resolution"

def process():
	is_windows = use_shell = os.name is 'nt' or sys.platform is 'win32'
	
	conf = dict()
	scriptloc = os.path.abspath(os.path.dirname(sys.argv[0]))
	f = os.path.splitext(os.path.basename(sys.argv[0]))[0] + ".conf"
	f = os.path.join(scriptloc, f)
	if os.path.exists(f):
		print "reading config data from", f
		handle_conf(open(f, "rU"), conf)
		print
	
	print "scanning script arguments"
	handle_conf(sys.argv[1:], conf)
	print
	
	log = conf.get('log')
	if log:
		print "using log:", log
		sys.stdout = sys.stderr = open(log, 'wt')
	del log
	
	impath = conf.get('imagemagick-path', '')
	identify_path = os.path.join(impath, 'identify')
	convert_path = os.path.join(impath, 'convert')
	
	exportpath = os.path.abspath(conf.get('export-path', '.'))
	importpath = os.path.abspath(conf.get('import-path', '.'))
	dir_contents = os.listdir(importpath)
	extensions = conf.get('extensions', '.png .pcx .tga').lower().split()
	try:
		resolutions = set(map(int, conf.get('resolutions', '16 32 64 128').split()))
	except ValueError:
		sys.exit("error: the 'resolutions' directive in", f, "must contain only base-ten integers")
	try:
		columns = int(conf.get('columns', 9))
	except ValueError:
		sys.exit("error: the 'columns' directive in", f, "must contain only base-ten integers")
	resolutions = list(resolutions)
	resolutions.sort()
	filter_increase = conf.get('filter-increase')
	if filter_increase == 'default': filter_increase = None
	filter_decrease = conf.get('filter-decrease')
	if filter_decrease == 'default': filter_decrease = None
	
	names = dict()
	for f in dir_contents:
		name, ext = os.path.splitext(f.lower())
		if ext not in extensions: continue
		fpath = os.path.join(importpath, f)
		print "fpath:", fpath
		pieces =  name.rsplit('.', 1)
		is_radar = False
		if len(pieces) == 2:
			if pieces[1].isdigit():
				name = pieces[0]
			if pieces[1] == 'radar':
				is_radar = True
				name = pieces[0]
		args = [identify_path, '-format', '%w %h', fpath]
		p = Popen(args, stdout=PIPE, stderr=PIPE, shell=use_shell)
		o = p.communicate()
		if p.returncode:
			print "args for identify:"
			print args
			print "ignoring %s: %s" % (f, o[1])
			continue
		w, h = map(int, o[0].split())
		if is_radar:
			if w != columns:
				print "ignoring %s: radar image is not", columns, "pixels wide." % f
				continue
			res = names.setdefault(name, [False] * (len(resolutions) + 1) + [h])
			if res[-2]:
				print "ignoring %s: radar image already found for %s" % (f, name)
				continue
			if res[-1] != h:
				print "ignoring %s: group has %i tile rows. this has %i rows" % \
					(f, res[-1], h)
				continue
			args = [convert_path, fpath, '-depth', '8', 'ppm:-']
			p = Popen(args, stdout=PIPE, stderr=PIPE, shell=use_shell)
			out = open(os.path.join(exportpath, name + '.radar'), 'wb')
			dump_to_radar(ds_ppm_parser(p.stdout), out)
			out.close()
			if p.wait():
				print "args for convert:"
				print args
				sys.exit("error while running convert on %s: %s" % \
					(f, p.stderr.read()))
			res[-2] = True
			print "using %s to generate the radar file for %s" % (f, name)
			continue
		tilesize, extra = divmod(w, columns)
		print f + ": tiles determined to be", tilesize, "pixels per dimension"
		if tilesize not in resolutions:
			print "ignoring %s: does not use one of the listed tile resolutions" % f
			continue
		fixed_w = tilesize * columns
		rows, extra = divmod(h, tilesize)
		fixed_h = tilesize * rows
		if fixed_h != h or fixed_w != w:
			print "ignoring %s: expected dimensions of %ix%i, but found %ix%i" % \
				(f, fixed_w, fixed_h, w, h)
			continue
		index = resolutions.index(tilesize)
		res = names.setdefault(name, [False] * (len(resolutions) + 1) + [rows])
		if rows != res[-1]:
			print "ignoring %s: group has %i tile rows. this has %i rows" % \
				(f, res[-1], h)
			continue
		if res[index]:
			print "ignoring %s: resolution already filled" % f
			continue
		dirpath = makedir(exportpath, "%s-%i" % (name, tilesize))
		print "splitting tiles from", f, "into", dirpath, "at", tilesize, "resolution"
		args = [convert_path, '-depth', '8']
		args.extend(['-crop', '%ix%i' % (tilesize, tilesize)])
		args.extend([fpath, os.path.join(dirpath, 'tile-%02d.png')])
		p = Popen(args, stderr=PIPE, shell=use_shell)
		o = p.communicate()
		if p.returncode:
			print "args for convert:"
			print args
			exit("error while running convert on %s: %s" % (f, o[1]))
		res[index] = True

	def filesortkey(name):
		try:
			return int(name[name.find('-') + 1:name.find('.')])
		except ValueError:
			return -1

	for name, levels in names.iteritems():
		for i, res in enumerate(resolutions):
			if not True in levels[:-2]: continue
			if levels[i]:
				if levels[-2]: continue
				out = open(os.path.join(exportpath, name + ".radar"), 'wb')
				args = [convert_path, None, '-sample', '1x1!', '-depth', '8', 'ppm:-']
				dirpath = os.path.join(exportpath, "%s-%i" % (name, res))
				files = os.listdir(dirpath)
				files.sort(key=filesortkey)
				print "generating radar file from files in", dirpath
				for f in files:
					if not f.endswith('.png'): continue
					args[1] = os.path.join(dirpath, f)
					p = Popen(args, stdout=PIPE, stderr=PIPE, shell=use_shell)
					dump_to_radar(ds_ppm_parser(p.stdout), out)
					if p.wait():
						print "args for convert:"
						print args
						sys.exit("error while running convert on %s: %s" % \
							(f, p.stderr.read()))
				out.close()
				levels[-2] = True
				continue
			dirpath = makedir(exportpath, "%s-%i" % (name, res))
			index, scale_down = nearest_resolution(i, levels[:-2])
			input_res = resolutions[index]
			input_dirpath = os.path.join(exportpath, "%s-%i" % (name, input_res))
			print "resizing tiles from", input_dirpath, "to", dirpath
			for f in os.listdir(input_dirpath):
				if not f.endswith('.png'): continue
				args = [convert_path, os.path.join(input_dirpath, f)]
				if scale_down:
					if filter_decrease:
						args.extend(['-filter', filter_decrease])
				elif filter_increase:
					args.extend(['-filter', filter_increase])
				args.extend(['-resize', "%ix%i!" % (res, res)])
				args.append(os.path.join(dirpath, f))
				p = Popen(args, stdout=PIPE, stderr=PIPE, shell=use_shell)
				if p.wait():
					print "args for convert:"
					print args
					sys.exit("error while running convert on %s: %s" % \
						(f, p.stderr.read()))

if __name__ == '__main__':
	process()
