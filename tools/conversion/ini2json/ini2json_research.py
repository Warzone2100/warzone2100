import ConfigParser
import json
import sys

if len(sys.argv) < 2:
	print 'Need file parameters'
	sys.exit(1)

config = ConfigParser.ConfigParser()
config.read(sys.argv[1])

def is_number(s):
	try:
		int(s)
		return True
	except ValueError:
		return False

data = {}
opts = ['researchPoints', 'researchPower', 'keyTopic', 'msgName', 'iconID', 'statID', 'requiredResearch', 'resultComponents', 
	'techCode', 'imdName', 'subgroupIconID', 'requiredStructures', 'redStructures', 'redComponents', 'resultComponents',
	'replacedComponents', 'resultStructures', 'name']
listopts = ['requiredResearch', 'resultComponents', 'requiredStructures', 'redStructures', 'redComponents', 'resultComponents', 'replacedComponents', 'resultStructures']
for section in config.sections():
	entry = {}
	for opt in opts:
		if config.has_option(section, opt):
			value = config.get(section, opt).strip()
			if value.startswith('"') and value.endswith('"'):
				value = value[1:-1]
			value = value.split(',')
			accum = []
			for result in value: # convert numbers
				if is_number(result):
					accum.append(int(result))
				else:
					accum.append(result)
			if not opt in listopts:
				assert len(accum) == 1, "Wrong number of items in %s:%s - %s" % (section, opt, str(accum))
				entry[opt] = accum[0]
			else:
				entry[opt] = accum # is a list
	if config.has_option(section, 'results'):
		value = config.get(section, 'results')
		value = value.split(',')
		accum = []
		for result in value: # convert custom format
			result = result.strip()
			if result.startswith('"'): result = result[1:]
			if result.endswith('"'): result = result[:-1]
			tmp = {}
			comps = result.split(':')
			if comps[0].isupper():
				tmp['class'] = 'Weapon'
				tmp['filterValue'] = comps[0]
				tmp['filterParameter'] = 'ImpactClass'
			elif comps[0] in ['Cyborgs', 'Droids', 'Transport']:
				tmp['class'] = 'Body'
				tmp['filterValue'] = comps[0]
				tmp['filterParameter'] = 'BodyClass'
			elif comps[0] in ['Wall', 'Structure', 'Defense']:
				tmp['class'] = 'Building'
				tmp['filterValue'] = comps[0]
				tmp['filterParameter'] = 'Type'
			elif comps[0] in ['RearmPoints', 'ProductionPoints', 'ResearchPoints', 'RepairPoints', 'Sensor', 'ECM', 'PowerPoints', 'Construct']:
				tmp['class'] = comps[0]
			else: # forgot something...
				print >> sys.stderr, 'Unknown filter in %s: %s (%s)' % (section, comps[0], str(comps))
				sys.exit(1)
			if len(comps) > 2:
				tmp['parameter'] = comps[1]
				tmp['value'] = int(comps[2])
			else:
				tmp['parameter'] = comps[0]
				tmp['value'] = int(comps[1])
			accum.append(tmp)
		entry['results'] = accum
	entry['id'] = section # for backwards compatibility
	assert not section in data, '%s conflicts' % section
	data[section] = entry
print json.dumps(data, indent=4, separators=(',', ': '), sort_keys=True)
