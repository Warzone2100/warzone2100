#!BPY
 
"""
Name: 'Warzone model (.pie)...'
Blender: 244
Group: 'Import'
Tooltip: 'Load a Warzone model file'
"""

__author__ = "Rodolphe Suescun, Gerard Krol, Kevin Gillette"
__url__ = ["blender"]
__version__ = "1.2"

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
from pie_common import *
import os, pie

#==================================================================================#
# Draws a thumbnail from a Blender image at x,y with sides that are edgelen long   #
#==================================================================================#

gen = None
ui_ref = None

def fresh_generator(filename):
	return pie.data_mutator(pie.parse(filename))

def seek_to_directive(err_on_nonmatch, fatal_if_not_found, *directives):
	for i in gen:
		type = i[pie.TOKEN_TYPE]
		if type == "directive" and i[pie.DIRECTIVE_NAME] in directives:
			return i
		elif type == "error":
			ui.debug(i[pie.ERROR].args[0], "error", i[pie.LINENO])
		elif err_on_nonmatch:
			ui.debug("expected %s directive." % directives[0], "warning", i[pie.LINENO])
	if fatal_if_not_found:
		ui.debug(directives[0] + " directive not found. cannot continue.",
			"fatal-error")

def generate_opt_gui(rect, optlist, tooltips, limits):
	""" expects rect to be [xmin, ymin, xmax, ymax] """

	BGL.glRecti(*rect)
	buttonwidth = 120
	buttonheight = 20
	margin = 5
	defbuttonwidth = 20 # default button
	defbuttonpos = rect[2] - defbuttonwidth - margin
	buttonpos = defbuttonpos - margin - buttonwidth
	labelwidth = buttonpos - rect[0] - margin
	numopts = len(optlist)
	for i, opt in enumerate(optlist):
		name = opt['name']
		val = scalar_value(opt['val'])
		posY = rect[3] - (i + 1) * (buttonheight + margin)
		if posY < rect[1] + margin: break
		Draw.PushButton("D", i + numopts, defbuttonpos, posY, defbuttonwidth, buttonheight,
			"Set this option to its default")
		tooltip = tooltips.get(name, "")
		if isinstance(opt['opts'], basestring):
			title = opt.get('title', "")
			if opt['opts'] is 'number':
				opt['val'] = Draw.Number(title, i, buttonpos, posY, buttonwidth,
					buttonheight, val, limits[name][0],
					limits[name][1], tooltip)
			elif opt['opts'] is 'bool':
				opt['val'] = Draw.Toggle(title, i, buttonpos, posY, buttonwidth,
					buttonheight, val, tooltip)
		else:
			numopts = len(opt['opts'])
			if numopts < 2:
				ui.debug("error: invalid option supplied to generate_opt_gui")
				continue
			if numopts is 2:
				Draw.PushButton(opt['opts'][val], i, buttonpos, posY,
					buttonwidth, buttonheight, tooltip)
			else:
				menustr = opt.get('title', "")
				if menustr: menustr += "%t|"
				menustr += '|'.join("%s %%x%i" % (opt['opts'][j], j) for j in xrange(numopts))
				opt['val'] = Draw.Menu(menustr, i, buttonpos, posY, buttonwidth,
					buttonheight, val, tooltip)
		Draw.Label(opt['label'], rect[0] + margin, posY, labelwidth, buttonheight)

def opt_process(ui):
	ui.setData('defaults/script', {'auto-layer': True, 'import-scale': 1.0,
		'scripts/layers': "pie_levels_to_layers.py",
		'scripts/validate': "pie_validate.py"})
	ui.setData('defaults/user', Registry.GetKey('warzone_pie', True) or dict())
	ui.setData('limits', {'import-scale': (0.1, 1000.0)})
	ui.setData('tooltips', {'auto-layer': "Selecting this would be the same as selecting all the newly created objects and running Object -> Scripts -> \"PIE levels -> layers\"",
		'import-scale': "Values under 1.0 may be useful when importing from a model edited at abnormally large size in pie slicer. Values over 1.0 probably won't be useful.",
		'scripts/layers': "Basename of the \"PIE levels -> layers\" script. Do not include the path. Whitespace is not trimmed",
		'scripts/validate': "Basename of the \"PIE validate\" script. Do not include the path. Whitespace is not trimmed"})

	i = seek_to_directive(True, False, "PIE")
	if i is None:
		ui.debug("not a valid PIE. PIE directive is required", "warning")
		gen = fresh_generator(ui.getData('filename'))
		ui.setData('pie-version', 2, True)
	else:
		ui.setData('pie-version', i[pie.DIRECTIVE_VALUE], True)
		ui.debug("version %i pie detected" % ui.getData('pie-version'))
	i = seek_to_directive(True, False, "TYPE")
	if i is None:
		ui.debug("not a valid PIE. TYPE directive is required", "warning")
		gen = fresh_generator(ui.getData('filename'))
		ui.setData('pie-type', 0x200, True)
	else:
		ui.setData('pie-type', i[pie.DIRECTIVE_VALUE], True)
		ui.debug("type %i pie detected" % ui.getData('pie-type'))
	optlist = list()
	dirs = Get('uscriptsdir'), Get('scriptsdir')
	script = default_value(ui, 'scripts/layers')
	for d in dirs:
		if d and script in os.listdir(d):
			optlist.append({'name': "auto-layer",
				'label': "Assign levels to different layers?",
				'opts': 'bool', 'title': "Automatically Layer"})
			ui.setData('scripts/layers', os.path.join(d, script), True)
			break
	else:
		ui.debug("Could not find '%s'. automatic layering will not be available" % script)
	optlist.append({'name': "import-scale", 'label': "Scale all points by a factor.",
		'opts': 'number', 'title': "Scale"})
	for opt in optlist:
		opt.setdefault('val', default_value(ui, opt['name']))
	ui.setData('optlist', optlist)

def opt_draw(ui):
	optlist = ui.getData('optlist')
	numopts = len(optlist)
	Draw.PushButton("Cancel", numopts * 2 + 1, 68, 15, 140, 30, "Cancel the import operation")
	Draw.PushButton("Proceed", numopts * 2, 217, 15, 140, 30, "Confirm texpage selection and continue")
	Draw.PushButton("Save Defaults", numopts * 2 + 3, 68, 321, 140, 30, "Save options in their current state as the default")
	Draw.PushButton("Default All", numopts * 2 + 2, 217, 321, 140, 30, "Revert all options to their defaults")
	BGL.glClearColor(0.7, 0.7, 0.7, 1)
	BGL.glClear(BGL.GL_COLOR_BUFFER_BIT)
	BGL.glColor3f(0.8, 0.8, 0.8)
	BGL.glRecti(5, 5, 431, 411)
	BGL.glColor3f(0.7, 0.7, 0.7)
	BGL.glRecti(15, 361, 421, 401)
	generate_opt_gui([15, 55, 421, 311], optlist, ui.getData('tooltips'), ui.getData('limits'))
	BGL.glColor3i(0, 0, 0)
	text = ("General Options", "large")
	BGL.glRasterPos2i(int((406 - Draw.GetStringWidth(*text)) / 2 + 15), 377)
	Draw.Text(*text)

def opt_evt(ui, val):
	opts = ui.getData('optlist')
	numopts = len(opts)
	if val >= numopts * 2:
		val -= numopts * 2
		if val is 0:
			if not ui.getData('defaults/user'): save_defaults(ui)
			for opt in opts:
				ui.setData(opt['name'], scalar_value(opt['val']), True)
			return True
		elif val is 1:
			return False
		elif val is 2:
			if ui.getData('defaults/user'):
				def_src = Draw.PupMenu(
					"Source of default value %t|Script default|User default")
			else:
				def_src = 1
			if def_src > 0:
				for opt in opts:
					name = opt['name']
					if def_src is 1: opt['val'] = ui.getData('defaults/script')[name]
					elif def_src is 2: opt['val'] = default_value(ui, name)
					else: break
				Draw.Redraw()
		elif val is 3:
			save_defaults(ui)
		return
			
	if val < numopts:
		opt = opts[val]
		if not isinstance(opt, basestring):
			if len(opt['opts']) is 2:
				opt['val'] = abs(opt['val'] - 1) # toggle it between 0 and 1
	elif val < numopts * 2:
		opt = ui.getData('optlist')[val - numopts]
		name = opt['name']
		if name in ui.getData('defaults/user'):
			def_src = Draw.PupMenu(
				"Source of default value %t|Script default|User default")
		else:
			def_src = 1
		if def_src is 1: opt['val'] = ui.getData('defaults/script')[name]
		elif def_src is 2: opt['val'] = ui.getData('defaults/user')[name]
		else: return
	Draw.Redraw()

def thumbnailize(img, x, y, edgelen):
	try:
		# BGL.glClearColor(0.7, 0.7, 0.7, 1)
		# BGL.glClear(BGL.GL_COLOR_BUFFER_BIT)
		if img: BGL.glColor3f(0.0, 0.0, 0.0)
		else: BGL.glColor3f(1.0, 1.0, 1.0)
		BGL.glRecti(x, y, x + edgelen, y + edgelen)
	except NameError, AttributeError:
		ui.debug("unable to load BGL. will not affect anything, though")
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
	texpage_cache = ui_ref.getData('texpage-cache')
	options = ui_ref.getData('texpage-opts')
	if filename in texpage_cache:
		img = texpage_cache[filename]
	else:
		img = Image.Load(filename)
		texpage_cache[filename] = img
	if filename not in options:
		ui.getData('texpage-menu').val = len(options)
		options.append(filename)
		if 'texpage-dir' not in ui:
			ui.setData('texpage-dir', os.path.dirname(filename))
	return img

def texpage_process(ui):
	global gen
	ui.setData('texpage-menu', Draw.Create(0))
	ui.setData('texpage-opts', list())
	ui.setData('texpage-cache', dict())
	i = seek_to_directive(False, False, "NOTEXTURE", "TEXTURE")
	if i is None: # else assume NOTEXTURE and run it again after everything else
		ui.debug("not a valid PIE. Either a TEXTURE or NOTEXTURE directive is required", "warning")
		gen = fresh_generator(ui.getData('filename'))
		texfilename = ""
		ui.setData('texpage-width', 256, True)
		ui.setData('texpage-height', 256, True)
	else:
		texfilename = i[pie.TEXTURE_FILENAME]
		ui.setData('texpage-width', i[pie.TEXTURE_WIDTH], True)
		ui.setData('texpage-height', i[pie.TEXTURE_HEIGHT], True)
	
	basename, ext = os.path.splitext(texfilename.lower())
	namelen = len(basename) + len(ext)
	texpage_opts = ui.getData('texpage-opts')
	ui.debug('basename: ' + basename)
	ui.debug('ext: ' + ext)
	for d in (('..', 'texpages'), ('..', '..', 'texpages'), ('..', '..', '..', 'texpages')):
		d = os.path.join(ui.getData('dir'), *d)
		if not os.path.exists(d): continue
		ui.setData('texpage-dir', d)
		for fn in os.listdir(d):
			fnlower = fn.lower()
			if fnlower.startswith(basename):
				canonical = os.path.abspath(os.path.join(d, fn))
				if fnlower.endswith(ext) and len(fn) == namelen:
					texpage_opts.insert(0, canonical)
				else:
					texpage_opts.append(canonical)
	ui.debug('texpage options: ' + str(ui.getData('texpage-opts')))

def texpage_draw(ui):
	Draw.PushButton("Other texpage", 2, 15, 135, 140, 30, "Select another texpage from your filesystem")
	Draw.PushButton("Cancel", 1, 15, 95, 140, 30, "Cancel the import operation")
	Draw.PushButton("Proceed", 0, 15, 55, 140, 30, "Confirm texpage selection and continue")
	options = ui.getData('texpage-opts')
	numopts = len(options)
	menustr = "Texpages %t"
	menustr += ''.join("|%s %%x%i" % (options[i], i) for i in xrange(numopts))
	menustr += "|NOTEXTURE %%x%i" % numopts
	ui.setData('texpage-menu', Draw.Menu(menustr, 3, 15, 15, 406, 30,
		ui.getData('texpage-menu').val, "Select a texpage to bind to the model"))
	menu = ui.getData('texpage-menu')
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

def texpage_evt(ui, val):
	if 0 == val:
		options = ui.getData('texpage-opts')
		texpage_cache = ui.getData('texpage-cache')
		menu  = ui.getData('texpage-menu')
		msg = "selected texpage: "
		currentMat = Material.New('PIE_mat')
		texture = Texture.New()
		if menu.val < len(options):
			msg += options[menu.val]
			ui.debug("binding texture to texpage")
			texture.setType('Image')
			image = texpage_cache[options[menu.val]]
			texture.image = image
		else:
			msg += "NOTEXTURE"
			texture.setType('None')
			image = None
		ui.debug(msg)
		currentMat.setTexture(0, texture, Texture.TexCo.UV, Texture.MapTo.COL)
		ui.setData('material', currentMat, True)
		ui.setData('image', image, True)
		return True
	if 1 == val:
		return False
	elif 2 == val:
		Window.ImageSelector(new_texpage, "Select a texpage",
			ui.getData('texpage-dir', ui.getData('dir')))
		Draw.Redraw()
	elif 3 == val:
		Draw.Redraw()

def model_process(ui):
	i = seek_to_directive(True, True, "LEVELS")
	numlevels = i[pie.DIRECTIVE_VALUE]
	level, nbActualPoints, default_coords, mesh = 0, 0, None, None
	scale = ui.getData('import-scale') 
	if scale is not 1.0:
		ui.debug("scaling by a factor of %.1f" % scale)
	point_divisor = 128.0 / scale

	def new_point():
		mesh.verts.extend(*default_coords)
		ui.debug("patching invalid point or point reference: new point is at (%.1f, %.1f, %.1f)" % \
			tuple(default_coords), "error")
		default_coords[1] -= 0.1
		return nbActualPoints + 1

	if ui.getData('pie-version') >= 5:
		divisorX, divisorY = 1, 1
	else:
		divisorX = ui.getData('texpage-width')
		divisorY = ui.getData('texpage-height')
	scn = Scene.GetCurrent()	        # link object to current scene
	pieobj = scn.objects.new("Empty", "PIE_" + os.path.splitext(
		os.path.basename(ui.getData('filename')))[0].upper())
	while level < numlevels:
		i = seek_to_directive(True, True, "LEVEL")
		level += 1
		if level != i[pie.DIRECTIVE_VALUE]:
			ui.debug("LEVEL should have value of %i on line %i. reordered to %i" % \
				(level, i[pie.LINENO], level), "warning")
		mesh = Mesh.New('clean')
		mesh.materials += [ui.getData('material')]
		mesh.addUVLayer('base')
		mesh.addUVLayer('teamcolor_meta')
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
						ui.debug("point no. %i is not valid." % num, "error", i[pie.LINENO])
						nbActualPoints = new_point()
					else:
						ui.debug(i[pie.ERROR].args, "error")
						break
				else:
					num += 1
					x, y, z = i[pie.FIRST:]
					#todo: convert to Mesh code
					mesh.verts.extend(-x / point_divisor, -z / point_divisor, y / point_divisor)
				nbActualPoints += 1
			else:
				abandon_level = False
			if abandon_level:
				ui.debug("remaining data in this LEVEL cannot be trusted.", "error")
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
						ui.debug("polygon no. %i is not valid. omitting" % num, "error", i[pie.LINENO])
						continue
					else:
						ui.debug(str(i[pie.ERROR].args[0]), "error", i[pie.LINENO])
						if isinstance(error, pie.PIEStructuralError):
							i = gen.next()
							if i[pie.TOKEN_TYPE] != "polygon":
								ui.debug("expected polygon data. abandoning this LEVEL", "error")
								break
							nbPoints, pos = i[pie.FIRST + 1], pie.FIRST + 2
							points = i[pos:pos + nbPoints]
							for p in (nbPoints - p for p in xrange(1, nbPoints)):
								if points.count(points[p]) > 1:
									i[pos + p], nbActualPoints = nbActualPoints, new_point()
									force_valid_points.append(p)
						else:	break
				if i[pie.TOKEN_TYPE] != "polygon":
					ui.debug("expected polygon data. abandoning this LEVEL", "error")
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
					mesh.activeUVLayer = 'teamcolor_meta'
					nbFrames = i[pos]
					framedelay = i[pos + 1]
					width = i[pos + 2]
					height = i[pos + 3]
					pos += 4
					ui.debug('max ' + str(nbFrames))
					ui.debug('time ' + str(framedelay))
					ui.debug('width ' + str(width))
					ui.debug('height ' + str(height))
					if nbFrames < 1:
						ui.debug("maximum number of teamcolors/animation frames must be at least 1", "error")
					if nbFrames is 1:
						ui.debug("maximum number of teamcolors/animation frames should be greater than 1", "warning")
					width /= divisorX
					height /= divisorY
					f.uv = create_teamcolor_meta(nbPoints, width, height, nbFrames, framedelay) 
				mesh.activeUVLayer = 'base'
				f.image = ui.getData('image')
				if ui.getData('pie-version') >= 5:
					uv = [Mathutils.Vector(i[pos], i[pos + 1]) for pos in range(pos, pos + 2 * nbPoints, 2)]
				else:
					uv = [Mathutils.Vector(i[pos] / divisorX, 1 - i[pos + 1] / divisorY) for pos in range(pos, pos + 2 * nbPoints, 2)]
				ui.debug("UVs: " + repr(uv))
				f.uv = uv
				if flags & 0x2000:
					# double sided
					f.mode |= Mesh.FaceModes['TWOSIDE']
				if flags & 0x800:
					# transparent
					f.transp = Mesh.FaceTranspModes['ALPHA']
		except StopIteration: pass
		ob = scn.objects.new(mesh, 'LEVEL_%i' % level)
		mesh.flipNormals()
		pieobj.makeParent([ob], 0, 0)
	i = seek_to_directive(False, False, "CONNECTORS")
	if i is not None:
		num, numtotal = 0, i[pie.DIRECTIVE_VALUE]
		while num < numtotal:
			i = gen.next()
			if i[pie.TOKEN_TYPE] == "error":
				if isinstance(i[pie.ERROR], pie.PIESyntaxError) and \
					i[pie.ERROR_ASSUMED_TYPE] == "connector":
					num += 1
					ui.debug("connector no. %i is not valid. omitting" % num, "error", i[pie.LINENO])
					continue
				else:
					ui.debug(i[pie.ERROR].args[0], "error", i[pie.LINENO])
					break
			num += 1
			x, y, z = i[pie.FIRST:]
			#empty = scn.objects.new('Empty', "CONNECTOR_%i" % num)
			empty = scn.objects.new('Empty', "CONNECTOR_%i" % num)
			empty.loc = x / point_divisor, -y / point_divisor, z / point_divisor
			empty.setSize(0.15, 0.15, 0.15)
			pieobj.makeParent([empty], 0, 0)
	if ui.getData('auto-layer'):
		ui.debug("layering all levels")
		Run(ui.getData('scripts/layers'))
	else:
		Redraw()
	return True

def load_pie(filename):
	global gen, ui_ref
	ui = BeltFedUI(True)
	ui_ref = ui
	ui.append(opt_process, opt_draw, opt_evt)
	ui.append(texpage_process, texpage_draw, texpage_evt)
	ui.append(model_process)
	gen = fresh_generator(filename)
	ui.setData('dir', os.path.dirname(filename), True)
	ui.setData('filename', filename, True)
	ui.Run()

Window.FileSelector(load_pie, 'Import Warzone model', '*.pie')
