#!/usr/bin/env python3
# encoding: utf-8

import json, sys, os
from funcs.wzjsonlocalizedstring import wz_localized_string_get_base_string

def print_string(s, out_file):
	out_file.write('_({})\n'.format(json.dumps(s, ensure_ascii=False)))

def parse_wz_guide_json_string(value, filename, out_file, jsonPath):
	try:
		base_en_str = wz_localized_string_get_base_string(value)
	except ValueError as e:
		raise ValueError('Invalid localized string [{0}]: {1}'.format(jsonPath, e.message))

	if base_en_str:
		print_string(base_en_str, out_file)

def parse_wz_guide_json_contents_array(contents_arr, filename, out_file, jsonPath='$.contents'):
	if not isinstance(contents_arr, list):
		raise ValueError('Invalid contents value type - expected list')
	for idx, v in enumerate(contents_arr):
		parse_wz_guide_json_string(v, filename, out_file, jsonPath + '[' + str(idx) + ']')

def parse_wz_guide_json(input_path, output_folder):

	obj = json.load(open(input_path, 'r'))

	if not isinstance(obj, dict):
		raise ValueError('WZ Guide JSON root is not an object')

	if not 'id' in obj:
		raise ValueError('WZ Guide JSON is missing required \"id\"')

	if not 'title' in obj:
		raise ValueError('WZ Guide JSON is missing required \"title\"')

	sanitized_id = obj['id'].replace('::', '-').replace(':', '-')
	output_filename = os.path.join(output_folder, sanitized_id + '.txt')

	with open(output_filename, 'w', encoding='utf-8') as out_file:
		if 'title' in obj:
			try:
				base_title_str = wz_localized_string_get_base_string(obj['title'])
			except ValueError as e:
				raise ValueError('Invalid title string [{0}]: {1}'.format('$.title', e.message))

			if base_title_str:
				out_file.write('// TRANSLATORS:\n')
				out_file.write('// The guide topic title - please maintain original capitalization\n')
				print_string(base_title_str, out_file)

		if 'contents' in obj:
			parse_wz_guide_json_contents_array(obj['contents'], input_path, out_file, '$.contents')

input_path = sys.argv[1]
output_folder = sys.argv[2]
parse_wz_guide_json(input_path, output_folder)
