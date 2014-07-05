import ConfigParser
import json
import sys

if len(sys.argv) < 3:
	print 'Need body.ini and bodypropulsionimd.ini parameters'
	sys.exit(1)

config = ConfigParser.ConfigParser()
config.optionxform = str # stop making keys lowercase
config.read(sys.argv[1])

config2 = ConfigParser.ConfigParser()
config2.optionxform = str # stop making keys lowercase
config2.read(sys.argv[2])

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
	entry = {}
	for opt in config.items(section):
		key = opt[0]
		if key in translation:
			key = translation[key]
		value = opt[1].strip('" ').split(',')
		accum = []
		for result in value: # convert numbers
			if is_number(result):
				accum.append(int(result))
			elif '.PIE' in result:
				accum.append(result.lower())
			else:
				accum.append(result)
		assert len(accum) == 1, "Wrong number of items in %s:%s - %s" % (section, opt, str(accum))
		entry[key] = accum[0]
		if config2.has_section(section):
			propmodels = {}
			for l in config2.items(section):
				models = {}
				m = l[1].strip(' "').split(',')
				if len(m) > 0 and m[0].strip('" ') != '0': models['left'] = m[0].strip('" ').lower()
				if len(m) > 1 and m[1].strip('" ') != '0': models['right'] = m[1].strip('" ').lower()
				if len(m) > 2 and m[2].strip('" ') != '0': models['still'] = m[2].strip('" ').lower()
				if len(m) > 3 and m[3].strip('" ') != '0': models['moving'] = m[3].strip('" ').lower()
				key2 = l[0]
				if key2 in translation:
					key2 = translation[key2]
				propmodels[key2] = models
			entry['propulsionExtraModels'] = propmodels
	entry['id'] = section
	assert not section in data, '%s conflicts' % section
	data[section] = entry
print json.dumps(data, indent=4, separators=(',', ': '), sort_keys=True)
