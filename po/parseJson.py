import json, sys, re

def printString(s, begin, end, filename, jsonPath):
	if not re.match(r'^(\*.*\*|CAM[0-9] .*|Z ?NULL.*)$', s):
		sys.stdout.write('{}{}) // SRC: {}: {}{}'.format(begin, json.dumps(s, ensure_ascii=False), filename, jsonPath, end))

def parse(obj, filename, jsonPath="$"):
	if isinstance(obj, dict):
		for k, v in obj.items():
			parse(v, filename, jsonPath + "." + k)
			if k in ['name', 'tip', 'easy_tip', 'medium_tip', 'hard_tip', 'insane_tip'] and isinstance(v, str):
				printString(v, '_(', '\n', filename, jsonPath + "." + k)
			elif k in ['text', 'ranks'] and isinstance(v, list):
				for s in v:
					if isinstance(s, str):
						if k == 'text':
							printString(s, '_(', '\n', filename, jsonPath + "." + k)
						elif k == 'ranks':
							printString(s, 'NP_("rank", ', '\n', filename, jsonPath + "." + k)
	elif isinstance(obj, list):
		for idx, v in enumerate(obj):
			parse(v, filename, jsonPath + "[" + str(idx) + "]")

parse(json.load(open(sys.argv[1], 'r')), sys.argv[1])
