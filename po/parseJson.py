import json, sys, re

def printString(s, begin, end):
	if not re.match(r'^(\*.*\*|CAM[0-9] .*|Z ?NULL.*)$', s):
		sys.stdout.write('{}{}){}'.format(begin, json.dumps(s, ensure_ascii=False), end))

def parse(obj):
	if isinstance(obj, dict):
		for k, v in obj.items():
			parse(v)
			if k == 'name' and isinstance(v, str):
				printString(v, '_(', '\n')
			elif k in ['text', 'ranks'] and isinstance(v, list):
				for s in v:
					if isinstance(s, str):
						if k == 'text':
							printString(s, '_(', '\n')
						elif k == 'ranks':
							printString(s, 'NP_("rank", ', '\n')
	elif isinstance(obj, list):
		for v in obj:
			parse(v)

parse(json.load(open(sys.argv[1], 'r')))
