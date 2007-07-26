__author__ = "Kevin Gillette"
__version__ = "0.1"

#
# --------------------------------------------------------------------------
# pie_common  v0.1 by Kevin Gillette (kage)
# --------------------------------------------------------------------------
# ***** BEGIN GPL LICENSE BLOCK *****
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
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# ***** END GPL LICENCE BLOCK *****
# --------------------------------------------------------------------------

import Blender

verbose = (Blender.Registry.GetKey('General') or dict()).get('verbose', True)

debug_messages = {
	'fatal-error': ["FATAL ERROR: ", 0],
	'error': ["ERROR: ", 0],
	'warning': ["Warning: ", 0],
	'notice': ["", 0]
}

debug_buffer = "" # also includes notices
error_list = list()

class FatalError(Exception):
	pass

def debug(message, type="notice", lineno=None):
	if type not in debug_messages: raise ValueError("not a correct message type")
	if lineno: message += " line: %i" % lineno
	tobj = debug_messages[type]
	tobj[1] += 1
	lbuf = tobj[0] + message
	if verbose: print lbuf
	global debug_buffer
	debug_buffer += lbuf + "\n"
	if type != "notice": error_list.append(lbuf)
	if type == 'fatal-error':
		raise FatalError

def scalar_value(val, type=None):
	if hasattr(val, 'val'):
		val = val.val
	if type is 'bool': val = bool(val)
	return val

def default_value(data, key):
	return data['defaults/user'].get(key, data['defaults/script'].get(key))

def save_defaults(data):
	defaults = data['defaults/script']
	defaults.update(data['defaults/user'])
	for opt in data['optlist']:
		name = opt['name']
		defaults[name] = scalar_value(opt['val'], opt['opts'])
	if 'tooltips' not in defaults: defaults['tooltips'] = dict()
	if 'limits' not in defaults: defaults['limits'] = dict()
	defaults['tooltips'].update(data['tooltips'])
	defaults['limits'].update(data['limits'])
	Blender.Registry.SetKey('warzone_pie', defaults, True)
	Blender.Draw.PupMenu("Defaults saved")

