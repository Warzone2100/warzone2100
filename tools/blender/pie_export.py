#!BPY
#from __future__ import division

"""
Name: 'Warzone model (.pie)...'
Blender: 244
Group: 'Export'
Tooltip: 'Save a Warzone model file'
"""

__author__ = "Gerard Krol"
__version__ = "1.0"
__bpydoc__ = """\
This script exports Warzone 2100 PIE files.
"""

#
# --------------------------------------------------------------------------
# PIE Export v0.1 by Gerard Krol (gerard_)
#            v0.2 by Kevin Gillette (kage)
#            v1.0 --
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
import os
import math
from pie_common import *

def fs_callback(ui, filepath, levels, connectors):
	texture_precision = point_precision = ui.getData('precision')
	if point_precision > 0:
		point_format = "\t" + (" %%.%if" % point_precision * 3)[1:] + "\n"
		uv_format = " %%.%if" % texture_precision * 2
		pie_version = 5
	else:
		point_format = "\t%i %i %i\n"
		uv_format = " %i %i"
		pie_version = 2
	out = file(filepath, 'w')
	out.write( "PIE %i\n" % pie_version )
	out.write( 'TYPE 200\n' )
	texture = levels[0].getMaterials()
	if len(texture): texture = texture[0]
	else: texture = levels[0].getData(mesh=1).faces[0].image
	if texture:
		texturename = os.path.basename(texture.getFilename())
		if texturename.count('.') is not 1:
			ui.debug("texture names in warzone can only have one period. " + filepath, 'error')
	else:
		texturename = 'NOTEXTURE'
		out.write("NO")
	ui.debug('texture: ' + texturename)
	if pie_version is 2:
		out.write("TEXTURE 0 %s 256 256\n" % texturename)
		texture_mult = 256.0
	else:
		if texture: x, y = texture.getSize()
		else: x, y = 256, 256
		out.write("TEXTURE 0 %s %i %i\n" % (texturename, x, y))
		texture_mult = 1.0
	out.write("LEVELS %i\n" % len(levels))
	for i, level in enumerate(levels):
		mesh = level.getData(mesh=True)
		mesh.flipNormals()
		out.write('LEVEL %i\n' % (i + 1))
		out.write('POINTS %i\n' % len(mesh.verts))
		for vert in mesh.verts:
			out.write(point_format % (-vert.co.x * 128, vert.co.z * 128, -vert.co.y * 128))
		out.write("POLYGONS %i\n" % len(mesh.faces))
		for j, face in enumerate(mesh.faces):
			line_buffer = str(len(face.verts))
			line_buffer += ''.join([" " + str(vert.index) for vert in face.verts])
			flags = 0
			mesh.activeUVLayer = "teamcolor_meta"
			meta = get_teamcolor_meta(face.uv)
			mesh.activeUVLayer = "base"
			if meta:
				ui.debug("teamcolor meta: " + repr(meta))
				line_buffer += (" %i %i" + uv_format) % tuple(meta)
				flags |= 0x4000
			if face.mode & Blender.Mesh.FaceModes['TWOSIDE']:
				flags |= 0x2000
			if face.transp == Blender.Mesh.FaceTranspModes['ALPHA']:
				flags |= 0x800
			if not face.mode & Blender.Mesh.FaceModes['TEX']:
				ui.debug("face %i is not textured" % j, "error")
			flags |= 0x200
			for u, v in face.uv:
				u *= texture_mult
				if pie_version is 2:
					v = (1.0 - v) * texture_mult
				else:
					v *= texture_mult
				line_buffer += uv_format % (u, v)
			out.write("\t%s %s\n" % (hex(flags).split('x')[-1], line_buffer))
		mesh.flipNormals()
	# export the connectors
	numconnectors = len(connectors)
	if numconnectors:
		out.write('CONNECTORS %i\n' % numconnectors)
		for c in connectors:
			x, y, z = c.getLocation()
			out.write(point_format % (x * 128, -y * 128, z * 128))
	out.close()

def pie_sel_process(ui):
	pie_names, pie_parts, levels, connectors = list(), list(), list(), list()
	for ob in Object.Get():
		name = ob.getName()
		if name.startswith("PIE_"):
			pie_names.append(name)
			pie_parts.append([list(), list()])
		elif name.startswith("LEVEL_"):
			levels.append(ob)
		elif name.startswith("CONNECTOR_"):
			connectors.append(ob)
	if len(pie_names) == 0:
		Draw.PupMenu("Error %t|No valid PIE objects found")
#		return False
	for i, lst in enumerate((levels, connectors)):
		for ob in lst:
			parent = ob.getParent()
			if parent:
				name = parent.getName()
				if name in pie_names:
					index = pie_names.index(name)
					pie_parts[index][i].append(ob)
	sort_key = lambda ob: ob.getName()
	pie_errors = list()
	for i, children in enumerate(pie_parts):
		children[0].sort(key=sort_key)
		children[1].sort(key=sort_key)
		pie_errors.append(validate(children[0]))
	rows = min(11, len(pie_names))
	ui.setData('pie_selection', [Draw.Create(not i) for i in pie_errors])
	ui.setData('pie_filenames', [Draw.Create( \
		normalizeObjectName(name[4:].lower()) + '.pie') for name in pie_names])
	ui.setData('rows', rows)
	ui.setData("pie_errors", pie_errors)
	ui.setData("pie_names", pie_names)
	ui.setData("pie_parts", pie_parts)
	ui.setScrollRange(0, len(pie_names) - rows)

def pie_sel_draw(ui):
	pie_names = ui.getData('pie_names')
	pie_parts = ui.getData('pie_parts')
	num_pies = len(pie_names)
	Draw.PushButton("Cancel", num_pies + 1, 68, 10, 140, 30, "Cancel the import operation")
	Draw.PushButton("Proceed", num_pies, 217, 10, 140, 30, "Confirm texpage selection and continue")
	BGL.glClearColor(0.7, 0.7, 0.7, 1)
	BGL.glClear(BGL.GL_COLOR_BUFFER_BIT)
	BGL.glColor3f(0.8, 0.8, 0.8)
	BGL.glRecti(5, 5, 430, 390)
	BGL.glColor3f(0.7, 0.7, 0.7)
	BGL.glRecti(10, 355, 425, 385)
	BGL.glRecti(10, 45, 425, 325)
	scroll = ui.getData('scroll-position', 0)
	pie_errors = ui.getData('pie_errors')
	pie_selection = ui.getData('pie_selection')
	pie_filenames = ui.getData('pie_filenames')
	rows = ui.getData('rows')
	ypos = 301
	row_height = 20
	ymargin = 5
	for i in xrange(scroll, scroll + rows):
		if pie_errors[i]:
			Draw.PushButton('!! ' + pie_names[i], i, 15, ypos, 160, row_height,
				"cannot be exported with errors. click to view errors")
		else:
			pie_selection[i] = Draw.Toggle(pie_names[i], i, 15, ypos, 160, row_height,
				pie_selection[i].val, "select/deselect this object for export")
			pie_filenames[i] = Draw.String('', i, 180, ypos, 240, row_height,
				pie_filenames[i].val, 80, "filename to export")
		ypos -= row_height + ymargin
	dir = ui.getData('export-dir', Draw.Create(os.getcwd()))
	dir = Draw.String("dir: ", num_pies + 4, 10, 330, 340, 20, dir.val, 399)
	ui.setData('export-dir', dir)
	Draw.PushButton("Browse", num_pies + 3, 355, 330, 70, 20)
	BGL.glColor3i(0, 0, 0)
	text = ("Select PIEs to export", "large")
	BGL.glRasterPos2i(int((425 - Draw.GetStringWidth(*text)) / 2 + 10), 366)
	Draw.Text(*text)

def pie_sel_evt(ui, val):
	pie_errors = ui.getData('pie_errors')
	num_pies = len(pie_errors)
	if val >= num_pies:
		val -= num_pies
		if val == 0:
			dir = ui.getData('export-dir').val
			if not dir: dir = './'
			else:
				if not os.path.isdir(dir):
					Draw.PupMenu('Error %t|' + dir + ' is not a valid directory')
					return
			pie_sel = ui.getData('pie_selection')
			pie_parts = ui.getData('pie_parts')
			pie_filenames = ui.getData('pie_filenames')
			pies = dict()
			for i, name in enumerate(pie_filenames):
				if not pie_errors[i] and pie_sel[i].val:
					pies[name.val] = pie_parts[i]
			ui.setData('pies', pies, True)
			dir = ui.getData('export-dir').val
			ui.setData('export-dir', dir, True)
			return True
		elif val == 1:
			return False
		elif 3 == val:
			setdir = lambda path: ui.setData('export-dir', Draw.Create(path))
			Window.FileSelector(setdir, 'Select export path', '')
			Draw.Redraw()
		return
	pie_names = ui.getData('pie_names')
	if pie_errors[val]:
		Draw.PupMenu("Errors in " + pie_names[val] + " %t|" + \
			'|'.join(["%s: %s" % err for err in pie_errors[val]]))
		return
	if not ui.getData('pie_filenames')[val].val:
		ui.getData('pie_selection')[val].val = 0
		Draw.Redraw()

def opts_draw(ui):
	precision = ui.getData('precision', Draw.Create(0))
	if precision.val > 0: pie_version = 5
	else: pie_version = 2
	Blender.Draw.Label("Version: PIE %i" % pie_version, 10, 80, 130, 30)
	precision = Blender.Draw.Number("precision", 2, 10, 50, 130, 30,
		precision.val, 0, 5)
	Blender.Draw.Button("Export", 0, 10, 10, 60, 30)
	Blender.Draw.Button("Cancel", 1, 80, 10, 60, 30)
	ui.setData('precision', precision)

def opts_evt(ui, val):
	if 0 == val:
		precision = ui.getData('precision').val
		ui.debug("starting export at floating-point precision of %i" % precision)
		ui.setData('precision', precision, True)
		return True
	elif 1 == val:
		return False
	else:
		Draw.Redraw()

def export_process(ui):
	dir = ui.getData('export-dir')
	for name, parts in ui.getData('pies').iteritems():
		name = os.path.join(dir, name)
		ui.debug('exporting to ' + name)
		fs_callback(ui, name, *parts)

ui = BeltFedUI(True)
ui.append(pie_sel_process, pie_sel_draw, pie_sel_evt)
ui.append(None, opts_draw, opts_evt)
ui.append(export_process)
ui.Run()
