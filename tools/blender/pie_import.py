#!BPY
 
"""
Name: 'Warzone model (.pie)...'
Blender: 244
Group: 'Import'
Tooltip: 'Load a Warzone model file'
"""

__author__ = "Rodolphe Suescun, Gerard Krol, Kevin Gillette"
__url__ = ["blender"]
__version__ = "1.0"

__bpydoc__ = """\
This script imports PIE files to Blender.

Usage:

Run this script from "File->Import" menu and then load the desired PIE file.
"""

#
# --------------------------------------------------------------------------
# PIE Import v0.1 by Rodolphe Suescun (AKA RodZilla)
#            v0.2 by Gerard Krol (gerard_)
#            v0.3 by Kevin Gillette (kage)
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

from Blender import *
import os, pie

#==================================================================================#
# Draws a thumbnail from a Blender image at x,y with sides that are edgelen long   #
#==================================================================================#

gen = None
mode = 0
modes = None
data = dict()

debug_messages = {
	'fatal-error': ["FATAL ERROR: ", 0],
	'error': ["ERROR: ", 0],
	'warning': ["Warning: ", 0],
	'notice': ["", 0]
}

debug_buffer = "" # also includes notices
error_list = []

class FatalError(Exception):
	pass

def debug(message, type="notice", lineno=None):
	if type not in debug_messages: raise ValueError("not a correct message type")
	if lineno: message += " line: %i" % lineno
	tobj = debug_messages[type]
	tobj[1] += 1
	lbuf = tobj[0] + message
	print lbuf
	global debug_buffer
	debug_buffer += lbuf + "\n"
	if type != "notice": error_list.append(lbuf)
	if type == 'fatal-error':
		raise FatalError

def fresh_generator(filename):
	return pie.data_mutator(pie.parse(filename))

def seek_to_directive(err_on_nonmatch, fatal_if_not_found, *directives):
	for i in gen:
		type = i[pie.TOKEN_TYPE]
		if type == "directive" and i[pie.DIRECTIVE_NAME] in directives:
			return i
		elif type == "error":
			debug(i[pie.ERROR].args[0], "error", i[pie.LINENO])
		elif err_on_nonmatch:
			debug("expected %s directive." % directives[0], "warning", i[pie.LINENO])
	if fatal_if_not_found:
		debug(directives[0] + " directive not found. cannot continue.",
			"fatal-error")

def thumbnailize(img, x, y, edgelen):
	try:
		# BGL.glClearColor(0.7, 0.7, 0.7, 1)
		# BGL.glClear(BGL.GL_COLOR_BUFFER_BIT)
		if img: BGL.glColor3f(0.0, 0.0, 0.0)
		else: BGL.glColor3f(1.0, 1.0, 1.0)
		BGL.glRecti(x, y, x + edgelen, y + edgelen)
	except NameError, AttributeError:
		debug("unable to load BGL. will not affect anything, though")
	if not img: return
	width, height = img.getSize()
	edgelen = float(edgelen)
	if width > height: primary, secondary, reverse = width, height, False
	else: primary, secondary, reverse = height, width, True
	if primary < edgelen: zoom = 1.0
	else: zoom = 1.0 / (primary / edgelen)
	offset = int((edgelen - zoom * secondary) / 2)
	if zoom == 1.0: offset = [int((edgelen - zoom * primary) / 2), offset]
	else: offset = [0, offset]
	if reverse: offset.reverse()
	Draw.Image(img, offset[0] + x, offset[1] + y, zoom, zoom)

def new_texpage(filename):
	texpage_cache, options = data['texpage-cache'], data['texpage-opts']
	if filename in texpage_cache:
		img = texpage_cache[filename]
	else:
		img = Image.Load(filename)
		texpage_cache[filename] = img
	if filename not in options:
		data['texpage-menu'].val = len(options)
		options.append(filename)
		if 'texpage-dir' not in data:
			data['texpage-dir'] = os.path.dirname(filename)
	return img

def texpage_process():
	global gen
	data['texpage-menu'] = Draw.Create(0)
	data['texpage-opts'] = list()
	data['texpage-cache'] = dict()
	i = seek_to_directive(True, False, "PIE")
	if i is None:
		debug("not a valid PIE. PIE directive is required", "warning")
		gen = fresh_generator(data['filename'])
		data['pie-version'] = 2
	else:
		data['pie-version'] = i[pie.DIRECTIVE_VALUE]
	i = seek_to_directive(False, False, "NOTEXTURE", "TEXTURE")
	if i is None: # else assume NOTEXTURE and run it again after everything else
		debug("not a valid PIE. Either a TEXTURE or NOTEXTURE directive is required", "warning")
		gen = fresh_generator(data['filename'])
		texfilename = ""
		data['texpage-width'] = 256
		data['texpage-height'] = 256
	else:
		texfilename = i[pie.TEXTURE_FILENAME]
		data['texpage-width'] = i[pie.TEXTURE_WIDTH]
		data['texpage-height'] = i[pie.TEXTURE_HEIGHT]
	
	basename, ext = os.path.splitext(texfilename.lower())
	namelen = len(basename) + len(ext)
	debug('basename: ' + basename)
	debug('ext: ' + ext)
	for d in (('..', 'texpages'), ('..', '..', 'texpages')):
		d = os.path.join(data['dir'], *d)
		if not os.path.exists(d): continue
		data['texpage-dir'] = d
		for fn in os.listdir(d):
			fnlower = fn.lower()
			if fnlower.startswith(basename):
				canonical = os.path.abspath(os.path.join(d, fn))
				if fnlower.endswith(ext) and len(fn) == namelen:
					data['texpage-opts'].insert(0, canonical)
				else:
					data['texpage-opts'].append(canonical)
	debug('texpage options: ' + str(data['texpage-opts']))

def texpage_draw():
	Draw.PushButton("Other texpage", 2, 15, 135, 140, 30, "Select another texpage from your filesystem")
	Draw.PushButton("Cancel", 1, 15, 95, 140, 30, "Cancel the import operation")
	Draw.PushButton("Proceed", 0, 15, 55, 140, 30, "Confirm texpage selection and continue")
	options = data['texpage-opts']
	numopts = len(options)
	menustr = "Texpages %t"
	menustr += ''.join("|%s %%x%i" % (options[i], i) for i in xrange(numopts))
	menustr += "|NOTEXTURE %%x%i" % numopts
	data['texpage-menu'] = Draw.Menu(menustr, 3, 15, 15, 406, 30, data['texpage-menu'].val, "Select a texpage to bind to the model")
	menu = data['texpage-menu']
	if menu.val < numopts:
		selected = options[menu.val]
		selected = new_texpage(selected)
		w, h = selected.getSize()
		Draw.Label("%ix%i at %i bpp" % (w, h, selected.getDepth()), 16, 289, 136, 20)
	else:
		Draw.Label("model will appear white", 16, 289, 136, 20)
		selected = None
	BGL.glClearColor(0.7, 0.7, 0.7, 1)
	BGL.glClear(BGL.GL_COLOR_BUFFER_BIT)
	BGL.glColor3f(0.8, 0.8, 0.8)
	BGL.glRecti(5, 5, 431, 361)
	BGL.glColor3f(0.7, 0.7, 0.7)
	BGL.glRecti(15, 241, 155, 311)
	BGL.glRecti(15, 321, 421, 351)
	BGL.glColor3i(0, 0, 0)
	text = ("Texpage Selection", "large")
	BGL.glRasterPos2i(int((406 - Draw.GetStringWidth(*text)) / 2 + 15), 332)
	Draw.Text(*text)
	thumbnailize(selected, 165, 55, 256)

def texpage_evt(val):
	if 0 == val:
		options, texpage_cache, menu = data['texpage-opts'], data['texpage-cache'], data['texpage-menu']
		msg = "selected texpage: "
		currentMat = Material.New('PIE_mat')
		texture = Texture.New()
		if menu.val < len(options):
			msg += options[menu.val]
			debug("binding texture to texpage")
			texture.setType('Image')
			image = texpage_cache[options[menu.val]]
			texture.image = image
		else:
			msg += "NOTEXTURE"
			texture.setType('None')
			image = None
		debug(msg)
		currentMat.setTexture(0, texture, Texture.TexCo.UV, Texture.MapTo.COL)
		data['material'] = currentMat
		data['image'] = image
		return True
	if 1 == val:
		return False
	elif 2 == val:
		Window.ImageSelector(new_texpage, "Select a texpage", data.get('texpage-dir', data['dir']))
		Draw.Redraw()
	elif 3 == val:
		Draw.Redraw()

def model_process():
	i = seek_to_directive(True, True, "LEVELS")
	numlevels = i[pie.DIRECTIVE_VALUE]
	level, nbActualPoints, default_coords, mesh = 0, 0, None, None

	def new_point():
		mesh.verts.extend(*default_coords)
		debug("patching invalid point or point reference: new point is at (%.1f, %.1f, %.1f)" % \
			tuple(default_coords), "error")
		default_coords[1] -= 0.1
		return nbActualPoints + 1

	if data['pie-version'] >= 5:
		divisorX, divisorY = 1, 1
	else:
		divisorX, divisorY = data['texpage-width'], data['texpage-height']
	scn = Scene.GetCurrent()	        # link object to current scene
	while level < numlevels:
		i = seek_to_directive(True, True, "LEVEL")
		level += 1
		if level != i[pie.DIRECTIVE_VALUE]:
			debug("LEVEL should have value of %i on line %i" % \
				(level, i[pie.LINENO]), "warning")
		mesh = Mesh.New('PIE')
		mesh.materials.append(data['material'])
		mesh.addUVLayer('base') 
		mesh.addUVLayer('teamcolors')
		i = seek_to_directive(True, True, "POINTS")
		num, nbOfficialPoints, nbActualPoints = 0, i[pie.DIRECTIVE_VALUE], 0
		default_coords = [1.0, 0.5, 0.0]
		abandon_level = True
		try:
			while num < nbOfficialPoints:
				i = gen.next()
				if i[pie.TOKEN_TYPE] == "error":
					if isinstance(i[pie.ERROR], pie.PIESyntaxError) and \
						i[pie.ERROR_ASSUMED_TYPE] == "point":
						num += 1
						debug("point no. %i is not valid." % num, "error", i[pie.LINENO])
						nbActualPoints = new_point()
					else:
						debug(i[pie.ERROR].args, "error")
						break
				else:
					num += 1
					x, y, z = i[pie.FIRST:]
					#todo: convert to Mesh code
					mesh.verts.extend(x / 128, -z / 128, y / 128)
				nbActualPoints += 1
			else:
				abandon_level = False
			if abandon_level:
				debug("remaining data in this LEVEL cannot be trusted.", "error")
				continue
			i = seek_to_directive(True, True, "POLYGONS")
			num, numtotal = 0, i[pie.DIRECTIVE_VALUE]
			while num < numtotal:
				i = gen.next()
				force_valid_points = list()
				if i[pie.TOKEN_TYPE] == "error":
					error = i[pie.ERROR]
					if isinstance(error, pie.PIESyntaxError) and \
						i[pie.ERROR_ASSUMED_TYPE] == "polygon":
						num += 1
						debug("polygon no. %i is not valid. omitting" % num, "error", i[pie.LINENO])
						continue
					else:
						debug(str(i[pie.ERROR].args[0]), "error", i[pie.LINENO])
						if isinstance(error, pie.PIEStructuralError):
							i = gen.next()
							if i[pie.TOKEN_TYPE] != "polygon":
								debug("expected polygon data. abandoning this LEVEL", "error")
								break
							nbPoints, pos = i[pie.FIRST + 1], pie.FIRST + 2
							points = i[pos:pos + nbPoints]
							for p in (nbPoints - p for p in xrange(1, nbPoints)):
								if points.count(points[p]) > 1:
									i[pos + p], nbActualPoints = nbActualPoints, new_point()
									force_valid_points.append(p)
						else:	break
				if i[pie.TOKEN_TYPE] != "polygon":
					debug("expected polygon data. abandoning this LEVEL", "error")
					break
				flags = i[pie.FIRST]
				nbPoints = i[pie.FIRST + 1]
				pos = pie.FIRST + 2
				points = i[pos:pos + nbPoints]
				for p in xrange(nbPoints):
					if points[p] >= nbOfficialPoints and p not in force_valid_points:
						points[p], nbActualPoints = nbActualPoints, new_point()
				mesh.faces.extend(points, ignoreDups=True)
				if not flags & 0x200: continue
				pos += nbPoints
				f = mesh.faces[num]
				num += 1
				f.mat = 0
				if flags & 0x4000:
					maximum = i[pos]
					time = i[pos + 1]
					width = i[pos + 2]
					height = i[pos + 3]
					pos += 4
					debug('max ' + str(maximum))
					debug('time ' + str(time))
					debug('width ' + str(width))
					debug('height ' + str(height))
					end = pos + nbPoints * 2
					u = min(i[pos:end:2])
					v = min(i[pos + 1:end + 1:2])
					finalU = u + width * 7
					finalV = v
					while finalU >= divisorX:
						finalU -= divisorX
						finalV += height
					# team colors
					uv = [Mathutils.Vector((finalU - u + i[p]) / divisorX, 1 - (finalV - v + i[p + 1]) / divisorY) for p in range(pos, pos + 2 * nbPoints, 2)]
					debug("teamcolor UVs" + repr(uv))
					mesh.activeUVLayer = 'teamcolors'
					f.image = data['image']
					f.uv = uv
				mesh.activeUVLayer = 'base'
				f.image = data['image']
				uv = [Mathutils.Vector(i[pos] / divisorX, 1 - i[pos + 1] / divisorY) for pos in range(pos, pos + 2 * nbPoints, 2)]
				debug("UVs" + repr(uv))
				f.uv = uv
				if flags & 0x2000:
					# double sided
					mesh.activeUVLayer = 'base'
					f.mode |= Mesh.FaceModes['TWOSIDE']
					mesh.activeUVLayer = 'teamcolors'
					f.mode |= Mesh.FaceModes['TWOSIDE']
				if flags & 0x800:
					# transparent
					mesh.activeUVLayer = 'base'
					f.transp = Mesh.FaceTranspModes['ALPHA']
					mesh.activeUVLayer = 'teamcolors'
					f.transp = Mesh.FaceTranspModes['ALPHA']
		except StopIteration: pass
		mesh.activeUVLayer = 'base'
		ob = scn.objects.new(mesh, 'PIE_LEVEL_%i' % level)
		layers = [1]
		if level < 20: layers.append(level)
		ob.layers = layers

	i = seek_to_directive(False, False, "CONNECTORS")
	if i is not None:
		num, numtotal = 0, i[pie.DIRECTIVE_VALUE]
		while num < numtotal:
			i = gen.next()
			if i[pie.TOKEN_TYPE] == "error":
				if isinstance(i[pie.ERROR], pie.PIESyntaxError) and \
					i[pie.ERROR_ASSUMED_TYPE] == "connector":
					num += 1
					debug("connector no. %i is not valid. omitting" % num, "error", i[pie.LINENO])
					continue
				else:
					debug(i[pie.ERROR].args[0], "error", i[pie.LINENO])
					break
			num += 1
			x, y, z = i[pie.FIRST:]
			empty = Object.New('Empty', "CONNECTOR_%i" % num)

			# convert our number strings to floats
			empty.loc = x / 128, -y / 128, z / 128
			empty.setSize(0.15, 0.15, 0.15)
			scn.objects.link(empty)	 # link our new object into scene
	Redraw()
	return True

def dump_log(filename):
	f = open(filename, "w")
	f.write(debug_buffer)
	f.close()

def error_process():
	if error_list:
		modes[mode][1:] = [error_draw, error_evt]
		data['error-page'] = 0

def error_draw():
	Draw.PushButton("Save log", 1, 85, 370, 140, 30, "Save the log to a file, and then continue with loading")
	Draw.PushButton("Finish", 0, 235, 370, 140, 30, "Do not save the errors, and continue with loading")
	BGL.glClearColor(0.7, 0.7, 0.7, 1)
	BGL.glClear(BGL.GL_COLOR_BUFFER_BIT)
	BGL.glColor3f(0.8, 0.8, 0.8)
	BGL.glRecti(5, 5, 455, 405)
	BGL.glColor3f(0.7, 0.7, 0.7)
	BGL.glRecti(10, 45, 450, 365)
	BGL.glRecti(195, 15, 265, 35)
	lines_per_page = 20
	num_errors = len(error_list)
	parts = divmod(num_errors, lines_per_page)
	pages = parts[0]
	if parts[1]: pages += 1
	page = data['error-page']
	Draw.Label("%i / %i" % (page + 1, pages), 196, 16, 68, 18)
	if page > 0:
		Draw.PushButton("Previous Page", 2, 50, 10, 140, 30, "Save the log to a file, and then continue with loading")
	if page < pages - 1:
		Draw.PushButton("Next Page", 3, 270, 10, 140, 30, "Do not save the errors, and continue with loading")
	start = page * lines_per_page
	end = min(start + lines_per_page, num_errors)
	BGL.glColor3i(0, 0, 0)
	counter = 0
	while start + counter < end:
		BGL.glRasterPos2i(11, 355 - counter * 16)
		Draw.Text(error_list[start + counter])
		counter += 1

def error_evt(val):
	if val == 0:
		return True
	if val == 1:
		filename = data['filename'].rsplit('.', 1)[0] + ".log"
		Window.FileSelector(dump_log, 'Save import log', filename)
		return
	if val == 2: data['error-page'] -= 1
	elif val == 3: data['error-page'] += 1
	Draw.Redraw()

def gui_evt(val=None):
	retval = modes[mode][2](val)
	if retval is False:
		debug("Import aborted")
		Draw.Exit()
	elif retval is True:
		Draw.Exit()
		global mode
		while retval is True:
			mode += 1
			if mode >= len(modes): return
			try:
				retval = modes[mode][0]()
			except FatalError:
				mode = len(modes) - 2
				continue
			if retval is None and modes[mode][1]:
				Draw.Register(modes[mode][1], None, gui_evt)
				break

def load_pie(filename):
	global gen, modes
	modes = [[None, None, lambda val: True],
		[texpage_process, texpage_draw, texpage_evt],
		[model_process, None, None],
		[error_process, None, None]]
	gen = fresh_generator(filename)
	data['dir'] = os.path.dirname(filename)
	data['filename'] = filename
	gui_evt()

Window.FileSelector(load_pie, 'Import Warzone model')
