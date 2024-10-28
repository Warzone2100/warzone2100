#!/usr/bin/env python3
# encoding: utf-8

# Gets the base language ("en") value from a WZ Json Localized String
def wz_localized_string_get_base_string(value):
	if isinstance(value, dict):
		if 'en' in value:
			return value['en']
		else:
			raise ValueError('Invalid WZ Json localized string - is an object, but has no base "en" key')
	else:
		# non-objects are treated as a single string value that is always expected to be the base ("en") language
		return str(value)
