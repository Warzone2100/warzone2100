import json, sys, re

def printString(s, begin, end):
	if not re.match(r'^(\*.*\*|CAM[0-9] .*|Z ?NULL.*)$', s):
		print('{}_({}){}'.format(begin, json.dumps(s, ensure_ascii=False), end), end='')

def parse(obj):
	if isinstance(obj, dict):
		for k, v in obj.items():
			parse(v)
			if k == 'name' and isinstance(v, str):
				printString(v, '', '\n')
			elif k == 'text' and isinstance(v, list):
				print('ZZ', end='')
				for s in v:
					if isinstance(s, str):
						printString(s, ' ', '')
				print('')
	elif isinstance(obj, list):
		for v in obj:
			parse(v)

#parse(json.load(sys.stdin))
parse(json.load(open(sys.argv[1], 'r')))
