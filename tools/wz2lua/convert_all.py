#!/usr/bin/env python
import os
import re
import os.path
import sys

def find_files(dir, ends_with):
	files = []
	for dirpath, dirnames, filenames in os.walk(dir):
		for f in filenames:
			if f.endswith(ends_with):
				files.append(os.path.join(dirpath, f))
	return files

def convertdir(dir):
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
		lua_slo = os.path.join(os.path.dirname(path), os.path.basename(slofile.replace('.slo', '_logic.lua')))
		print 'processing', vlofile
		os.system("%s %s %s %s %s" % (os.path.normpath('tools/wz2lua/wz2lua.py'), vlofile, slofile, lua_vlo, lua_slo))

# remove the old files
for path in find_files(".", "_logic.lua"):
	os.remove(path)

if len(sys.argv) > 1:
	convertdir(sys.argv[1])
else:
	print 'no directory specified, converting everything'
	convertdir('data/base/script')
	convertdir('data/base/multiplay/script')
	convertdir('data/base/multiplay/skirmish')
	convertdir('data/mods/global/aivolution')
	convertdir('data/mp/multiplay/script')
