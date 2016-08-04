import ConfigParser
import json
import sys

if len(sys.argv) < 2:
	print 'Need file parameters'
	sys.exit(1)

config = ConfigParser.ConfigParser()
config.optionxform = str # stop making keys lowercase
config.read(sys.argv[1])

def is_number(s):
	try:
		int(s)
		return True
	except ValueError:
		return False

data = {}
listopts = ['position', 'rotation']
for section in config.sections():
	entry = {}
	for opt in config.items(section):
		key = opt[0]
		value = opt[1]
		if value.startswith('"') and value.endswith('"'):
			value = value[1:-1]
		value = value.split(',')
		accum = []
		for result in value: # convert numbers
			if is_number(result):
				accum.append(int(result))
			else:
				accum.append(result)
		if key in listopts:
			entry[key] = accum
		else:
			assert len(accum) == 1, "Wrong number of items in %s:%s - %s" % (section, opt, str(accum))
			entry[key] = accum[0]
		if opt[0] == 'name':
			entry['id'] = opt[1] # for backwards compatibility, keep untranslated version around
	assert not section in data, '%s conflicts' % section
	data[section] = entry
print json.dumps(data, indent=4, separators=(',', ': '), sort_keys=True)
