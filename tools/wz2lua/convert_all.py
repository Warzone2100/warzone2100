#!/usr/bin/env python
import os
import re
import os.path
import sys
from optparse import OptionParser

def find_files(dir, ends_with):
	files = []
	for dirpath, dirnames, filenames in os.walk(dir):
		for f in filenames:
			if f.endswith(ends_with):
				files.append(os.path.join(dirpath, f))
	return files

# dir = directory to process,  verbose is boolean flag

def convertdir(dir, verbose):
	print 'processing', dir, '...'
	# first make a dictonary to find the slofiles
	slo = {}
	for path in find_files(dir, ".slo"):
		name = os.path.basename(path).split('.')[0]
		if name in slo:
			print path, 'already in slo', slo[name]
		slo[name] = path

	# add extra ones
	for path in find_files(".", ".slo"):
		name = os.path.basename(path).split('.')[0]
		if not name in slo:
			slo[name] = path

	for path in find_files(dir, ".vlo"):
		vlofile = path
		slofile = ''
		# now try to find the corresponding slo file
		f = file(vlofile)
		contents = f.read()
		f.close()
		found = re.findall('script[ \t]+"(.+?).slo"', contents)
		if found:
			name = found[0].lower()
			#print 'grepped', name
			if name in slo:
				slofile = slo[name]
		if not slofile:
			# generate it from the filename
			name = os.path.basename(path).split('.')[0]
			#print 'generated', name
			if name in slo:
				slofile = slo[name]
			else:
				print 'WARNING: could not find slo for', vlofile

		lua_vlo = path.replace('.vlo', '.lua')
		lua_slo = os.path.join(os.path.dirname(path),
							   os.path.basename(slofile.replace('.slo',
																'_logic.lua')))
		print 'processing', vlofile
		cmd = os.path.normpath('tools/wz2lua/wz2lua.py')
		if verbose: cmd += ' -v'
		os.system("%s %s %s %s %s" % (cmd , vlofile, slofile, lua_vlo, lua_slo))


#
# main
#

usage = '''
utility to convert WZ AI scripts to lua
run from root of source tree
usage: tools/wz2lua/%prog [options] [directory to process]
'''
parser = OptionParser(usage=usage)
parser.add_option( '-v', '--verbose', dest='verbose', default = False,
				   action = 'store_true',
				   help='add extra comments in converted scripts')

(opts, args) = parser.parse_args()
## print 'opts=',opts, ' args = ', args

				   


# remove the old files
for path in find_files(".", "_logic.lua"):
	os.remove(path)

if args:
	convertdir(args[0], opts.verbose)
else:
	print 'no directory specified, converting everything'
	dirs = ['data/base/script',
			'data/base/multiplay/script',
			'data/base/multiplay/skirmish',
			'data/mods/global/aivolution',
			'data/mp/multiplay/script']
	for d in dirs:
		convertdir( d, opts.verbose)
