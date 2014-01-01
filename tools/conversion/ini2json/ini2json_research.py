import ConfigParser
import json
import sys

if len(sys.argv) < 3:
	print 'Need file and dictionary parameters'
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
fp = open(sys.argv[2])
ids = json.load(fp)
opts = ['researchPoints', 'researchPower', 'keyTopic', 'msgName', 'iconID', 'statID', 'requiredResearch', 'resultComponents', 
	'techCode', 'imdName', 'subgroupIconID', 'requiredStructures', 'redStructures', 'redComponents', 'resultComponents',
	'replacedComponents']
for section in config.sections():
	name = config.get(section, 'name')
	if name.startswith('"') and name.endswith('"'):
		name = name[1:-1]
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
				elif result in ids:
					accum.append(ids[result]) # change ID to real name
				else:
					accum.append(result)
			if len(accum) == 1:
				entry[opt] = accum[0]
			else:
				entry[opt] = accum
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
				tmp['weaponSubClass'] = comps[0]
			elif comps[0] in ['Cyborgs', 'Droids', 'Transport']:
				tmp['bodyClass'] = comps[0]
			elif comps[0] in ['Wall', 'Structure', 'Defense']:
				tmp['structureType'] = comps[0]
			elif comps[0] in ['RearmPoints', 'ProductionPoints', 'ResearchPoints', 'RepairPoints', 'Sensor', 'ECM', 'PowerPoints', 'Construct']:
				pass # affects a global state
			else: # forgot something...
				print 'Unknown filter: %s' % comps[0]
				sys.exit(1)
			if len(comps) > 2:
				tmp[comps[1]] = int(comps[2])
			else:
				tmp[comps[0]] = int(comps[1])
			accum.append(tmp)
		entry['results'] = accum
	data[name] = entry
	data['id'] = section # for backwards compatibility
fp.close()
print json.dumps(data, indent=4, separators=(',', ': '))
