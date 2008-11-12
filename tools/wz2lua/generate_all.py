#!/usr/bin/env python
import os
import re

def convertdir(dir):
	print 'processing', dir, '...'
	# first make a dictonary to find the slofiles
	os.system('find '+dir+' -name "*.slo" > slofiles')
	slofiles = file('slofiles')
	slo = {}
	for path in slofiles:
		name = path.split('/')[-1].split('.')[0]
		if name in slo:
			print path, 'already in slo', slo[name]
		slo[name] = path.strip()
	slofiles.close()
	
	# add extra ones
	os.system('find -name "*.slo" > slofiles')
	slofiles = file('slofiles')
	slo = {}
	for path in slofiles:
		name = path.split('/')[-1].split('.')[0]
		if not name in slo:
			slo[name] = path.strip()
	slofiles.close()

	os.system('find '+dir+' -name "*.vlo" > vlofiles')
	vlofiles = file('vlofiles')
	for path in vlofiles:
		vlofile = path.strip()
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
			name = path.split('/')[-1].split('.')[0]
			#print 'generated', name
			if name in slo:
				slofile = slo[name]
			else:
				print 'WARNING: could not find slo for', vlofile

		luafile = path.replace('.vlo', '.lua')
		print 'processing', vlofile, slofile
		os.system('tools/wz2lua/wz2lua.py '+vlofile+' '+slofile+' > '+luafile)
		
	vlofiles.close()
	
convertdir('data/base')
