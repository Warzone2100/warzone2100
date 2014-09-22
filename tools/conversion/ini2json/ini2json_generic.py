import ConfigParser
import json
import sys

if len(sys.argv) < 2:
	print 'Need file parameter'
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

translation = {
	# templates
	'compBody' : 'body', 'compPropulsion' : 'propulsion', 'compSensor' : 'sensor',
	'compConstruct' : 'construct', 'compRepair' : 'repair', 'compBrain' : 'brain'
        # heat vs thermal vs ... TODO
}

data = {}
listopts = ['structureModel', 'weapons', 'speedFactor', 'text', 'pos1', 'pos2', 'members', 'pos']
for section in config.sections():
	entry = {}
	for opt in config.items(section):
		key = opt[0]
		if key in translation:
			key = translation[key]
		value = opt[1]
		if value.startswith('_(') and value.endswith(')'): # remove translation markup
			value = value[2:-1]
		if value.startswith('"') and value.endswith('"'):
			value = value[1:-1]
		value = value.split(',')
		accum = []
		for result in value: # convert numbers
			if key == 'text':
				if result.startswith('\n"') and result.endswith('"'):
					accum.append(result[2:-1])
				else:
					accum.append(result)
			elif is_number(result):
				accum.append(int(result))
			else:
				accum.append(result)
		if key in listopts:
			entry[key] = accum
		else:
			assert len(accum) == 1, "Wrong number of items in %s:%s - %s" % (section, opt, str(accum))
			entry[key] = accum[0]
	entry['id'] = section
	assert not section in data, '%s conflicts' % section
	data[section] = entry
print json.dumps(data, indent=4, separators=(',', ': '), sort_keys=True)
