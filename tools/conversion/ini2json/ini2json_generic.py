import ConfigParser
import json
import sys

if len(sys.argv) < 3:
	print 'Need file and dictionary parameters'
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
fp = open(sys.argv[2])
ids = json.load(fp)
listopts = ['structureModel', 'weapons']
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
			elif result in ids:
				accum.append(ids[result]) # change ID to real name
			else:
				accum.append(result)
		if key in listopts:
			entry[key] = accum
		else:
			assert len(accum) == 1, "Wrong number of items in %s:%s - %s" % (section, opt, str(accum))
			entry[key] = accum[0]
	entry['id'] = section # for backwards compatibility
	data[name] = entry
fp.close()
print json.dumps(data, indent=4, separators=(',', ': '), sort_keys=True)
