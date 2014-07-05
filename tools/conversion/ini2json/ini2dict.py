import ConfigParser
import json
import sys
import os

if len(sys.argv) < 2:
	print 'Need file parameter(s)'
	sys.exit(1)

data = {}
reverse = {}
for n in range(1, len(sys.argv)):
	config = ConfigParser.ConfigParser()
	config.optionxform = str # stop making keys lowercase
	config.read(sys.argv[n])

	for section in config.sections():
		name = config.get(section, 'name')
		if name.startswith('"') and name.endswith('"'):
			name = name[1:-1]
		if name in reverse:
			print >> sys.stderr, "%s (%s) in %s is also defined in %s (%s)" % (name, section, os.path.basename(sys.argv[n]), reverse[name][0], reverse[name][1])
		else:
			reverse[name] = [ os.path.basename(sys.argv[n]), section ]
		if section in data:
			print >> sys.stderr, "Error: %s is duplicate in %s" % (section, sys.argv[n])
		data[section] = name
print json.dumps(data, indent=4, separators=(',', ': '))
