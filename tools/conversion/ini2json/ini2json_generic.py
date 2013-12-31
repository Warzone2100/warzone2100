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
for section in config.sections():
	name = config.get(section, 'name')
	if name.startswith('"') and name.endswith('"'):
		name = name[1:-1]
	entry = {}
	for opt in config.items(section):
		if opt[0] == 'name': continue
		key = opt[0]
		if key in translation:
			key = translation[key]
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
		if len(accum) == 1 and key != 'weapons':
			entry[key] = accum[0]
		else:
			entry[key] = accum
	data[name] = entry
print json.dumps(data, indent=4, separators=(',', ': '))
