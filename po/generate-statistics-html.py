#!/usr/bin/env python

# Warzone 2100 Translation Status Generator
# Copyright (C) 2012 Michal Dutkiewicz aka Emdek <emdeck@gmail.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

import glob
import os
import re
import subprocess

width = 300
images = 'http://static.wz2100.net/img/flags/'
messages = {}
percents = {}
catalogs = {}

for i in glob.glob('*.po'):
	process = subprocess.Popen(['msgfmt', '--statistics', i], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	errors, output = process.communicate()

	messages['translated'] = 0
	messages['untranslated'] = 0
	messages['fuzzy'] = 0
	messages['amount'] = 0

	matches = re.search('(\d+) translated', output)

	if matches != None:
		messages['translated'] = int(matches.group(1))

	matches = re.search('(\d+) untranslated', output)

	if matches != None:
		messages['untranslated'] = int(matches.group(1))

	matches = re.search('(\d+) fuzzy', output)

	if matches != None:
		messages['fuzzy'] = int(matches.group(1))

	messages['amount'] = (messages['translated'] + messages['untranslated'] + messages['fuzzy'])

	percents['fuzzy'] = (0 if messages['fuzzy'] == 0 else (float(messages['fuzzy']) / messages['amount']));
	percents['untranslated'] = (0 if messages['untranslated'] == 0 else (float(messages['untranslated']) / messages['amount']));
	percents['translated'] = (1 - percents['fuzzy'] - percents['untranslated']);

	catalog = '%5d%s' % (((1 - percents['translated']) * 1000), i)

	catalogs[catalog] = '<li class="' + ('translation_complete' if percents['translated'] >= 0.9 else ('translation_unfinished' if percents['translated'] >= 0.8 else 'translation_incomplete')) + '">\n<span class="messages_translated" style="width: ' + str(width) + 'px" title="' + output.rstrip()[0:-1] + '">\n'

	if percents['untranslated'] > 0:
		catalogs[catalog] += '<span class="messages_untranslated" style="width: %dpx"></span>\n' % max(2, (percents['untranslated'] * width))

	if percents['fuzzy'] > 0:
		catalogs[catalog] += '<span class="messages_fuzzy" style="width: %dpx"></span>\n' % max(2, (percents['fuzzy'] * width))

	catalogs[catalog] += '</span>\n<img src="%s%s.png" alt="" title="%s" /> <strong>%s</strong> %.2f%%\n</li>' % (images, i[0:-3], i[0:-3], i[0:-3], (percents['translated'] * 100))

keys = catalogs.keys()
keys.sort()

print '<ul id="translation_status">'

for i in keys:
	print catalogs[i]

print '</ul>'
