#!BPY

"""
Name: 'PIE levels -> layers'
Blender: 244
Group: 'Object'
Tooltip: 'Give each selected PIE LEVEL object its own visible layer'
"""

__author__ = "Kevin Gillette"
__url__ = ["blender"]
__version__ = "0.2"

__bpydoc__ = """\
All "clean" LEVEL_* objects that either are the selected objects or descendants
thereof will be given a separate layer, from levels 1-18, while "broken" level
objects will appear in layer 20. "broken" objects are ones that were built from
untrustable data, for more complete recovery from malformed PIEs, and should
have a "clean" counterpart that can be trusted.

Usage:

Select any number of objects and run Object -> Scripts -> "PIE levels -> layers"

For each selected object, if it has a PIE_* object as either its descendent or
ancestor, that PIE_* object and all LEVEL_* and CONNECTOR_* descendants will be
assigned to their respective layers, though if the 'strict_matching' option is
selected, then only those objects explicitly selected and matching the above
criteria will be assigned layers.
"""

#
# --------------------------------------------------------------------------
# PIE levels -> layers  v0.2 by Kevin Gillette (kage)
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
selobjects = Blender.Object.GetSelected()
pie_layers = dict()
pie_connectors = dict()
script_key = "pie_levels_to_layers"

def find_pie(obj):
	if obj is None: return None
	if obj.getName().startswith("PIE_"): return obj
	return find_pie(obj.getParent())

#for ob in Blender.Scene.GetCurrent().objects:
for ob in selobjects:
	name = ob.getName()
	if name.startswith("LEVEL_"):
		start = 6 # start looking for the level num after "LEVEL_"
		end = name.find(".", 7) # ignore .001 on the end
		if end is -1: end = len(name)
		try: level = int(name[start:end])
		except ValueError: continue
		layers = [1]
		if 0 < level < 19: layers.append(level + 1)
		ob.layers = layers
	elif name.startswith("BROKEN_LEVEL_"):
		layers = [20]
		ob.layers = layers
	elif name.startswith("CONNECTOR_"):
		name = find_pie(ob).getName()
		pie_connectors.setdefault(name, list()).append(ob)
		continue
	else:
		continue
	pie = find_pie(ob)
	if pie and pie in selobjects:
		pie_layers.setdefault(pie.getName(), set()).update(layers)
for name in pie_layers:
	ob, layers = Blender.Object.Get(name), list(pie_layers[name])
	ob.layers = layers
	if name in pie_connectors:
		for i in pie_connectors[name]:
			i.layers = layers
Blender.Redraw()
