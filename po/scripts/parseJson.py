import json, sys, re

IGNORED_WZ_JSON_FILE_TYPES = {'wz2100.proxmsgs.v1', 'wz2100.briefs.v1'}

def printString(s, begin, end, filename, jsonPath):
	if not re.match(r'^(\*.*\*|CAM[0-9] .*|Z ?NULL.*)$', s):
		sys.stdout.write('{}{}) // SRC: {}: {}{}'.format(begin, json.dumps(s, ensure_ascii=False), filename, jsonPath, end))

def parse(obj, filename):
	def _parse(obj, filename, jsonPath="$"):
		if isinstance(obj, dict):
			for k, v in obj.items():
				_parse(v, filename, jsonPath + "." + k)
				if k in ['name', 'category', 'tip', 'easy_tip', 'medium_tip', 'hard_tip', 'insane_tip'] and isinstance(v, str):
					printString(v, '_(', '\n', filename, jsonPath + "." + k)
				elif k in ['text', 'ranks'] and isinstance(v, list):
					for idx, s in enumerate(v):
						itemPath = jsonPath + "." + k + "[" + str(idx) + "]"
						if isinstance(s, str):
							if k == 'text':
								printString(s, '_(', '\n', filename, itemPath)
							elif k == 'ranks':
								printString(s, 'NP_("rank", ', '\n', filename, itemPath)
		elif isinstance(obj, list):
			for idx, v in enumerate(obj):
				_parse(v, filename, jsonPath + "[" + str(idx) + "]")
	
	if isinstance(obj, dict):
		# check for json format types that should be ignored
		if 'type' in obj:
			if obj['type'] in IGNORED_WZ_JSON_FILE_TYPES:
				#sys.stderr.write('// IGNORING: {}\n'.format(filename))
				return
	
	_parse(obj, filename);

parse(json.load(open(sys.argv[1], 'r')), sys.argv[1])
