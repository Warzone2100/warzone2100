__author__ = "Kevin Gillette"
__version__ = "1.0"

#
# --------------------------------------------------------------------------
# pie_common  v0.1 by Kevin Gillette (kage)
#             v1.0 --
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

verbose = (Registry.GetKey('General') or dict()).get('verbose', True)

class FatalError(Exception):
	pass

def scalar_value(val, type=None):
	if hasattr(val, 'val'):
		val = val.val
	if type is 'bool': val = bool(val)
	return val

def default_value(ui, key):
	return ui.getData('defaults/user').get(key, ui.getData('defaults/script').get(key))

def save_defaults(ui):
	defaults = ui.getData('defaults/script')
	defaults.update(ui.getData('defaults/user'))
	for opt in ui.getData('optlist'):
		name = opt['name']
		defaults[name] = scalar_value(opt['val'], opt['opts'])
	if 'tooltips' not in defaults: defaults['tooltips'] = dict()
	if 'limits' not in defaults: defaults['limits'] = dict()
	defaults['tooltips'].update(ui.getData('tooltips'))
	defaults['limits'].update(ui.getData('limits'))
	Registry.SetKey('warzone_pie', defaults, True)
	Draw.PupMenu("Defaults saved")

tc_unit_size = 100

def get_teamcolor_meta(uvs):
	for coord in uvs[:3]:
		if coord[0] not in (0.0, 1.0) or coord[1] not in (0.0, 1.0): break
	else: return None
	width, height = uvs[0]
	frames = round((uvs[1][0] - width) * tc_unit_size)
	delay = round((uvs[2][1] - height) * tc_unit_size)
	return [frames, delay, width, height]

def create_teamcolor_meta(nbPoints, width, height, frames, delay=10):
	offsetU = width + frames / tc_unit_size
	offsetV = height + delay / tc_unit_size
	offsets = range(nbPoints - 3)
	offsets.reverse()
	uv = [Mathutils.Vector(width, height),
		Mathutils.Vector(offsetU, height),
		Mathutils.Vector(offsetU, offsetV)]
	uv.extend([Mathutils.Vector(width, offsetV + 0.01 * i) for i in offsets])
	return uv

class BeltFedUI:
	def __init__(self, debug_gui=False):
		self._persistent_data = dict()
		self._volatile_data = dict()
		self._beltlinks = [[None, None, lambda self, val: True, None]] # used for bootstrapping
		self._linknames = dict()
		self._index = -1
		self._skipui = False
		self._debug_gui = debug_gui

		self._debug_messages = {
			'fatal-error': ["FATAL ERROR: ", 0],
			'error': ["ERROR: ", 0],
			'warning': ["Warning: ", 0],
			'notice': ["", 0]
		}

		self._debug_buffer = "" # also includes notices
		self._error_list = list()

	def debug(self, message, type="notice", lineno=None):
		if type not in self._debug_messages:
			raise ValueError("not a correct message type")
		if lineno: message += " line: %i" % lineno
		tobj = self._debug_messages[type]
		tobj[1] += 1
		lbuf = tobj[0] + message
		if verbose: print lbuf
		self._debug_buffer += lbuf + "\n"
		if type != "notice": self._error_list.append(lbuf)
		if type == 'fatal-error':
			raise FatalError()

	def append(self, process, draw=None, event=None, scrl_range=None, name=None):
		if self._index >= 0:
			raise ValueError("cannot append a beltlink after process has started")
		if (draw or event) and not (draw and event):
			raise TypeError("Both draw and event should be callable or both should be None")
		self._beltlinks.append([process, draw, event, scrl_range])
		if name: self._linknames[name] = len(self._beltlinks)

	def setScrollRange(self, min, max):
		if self._index < 0:
			raise ValueError("cannot set the scroll range before the process has started.")
		if min == max:
			self._beltlinks[self._index][3] = None
			return
		if min > max:
			raise ValueError("min must be less than max")
		self._beltlinks[self._index][3] = (min, max)
	
	def skipUI(self):
		self._skipui = True

	def jumpToNamedLink(name):
		if self._index < 0: raise ValueError("must first start the ui process")
		if name in self._linknames:
			self._index = self._linknames[name]
			return True
		return False

	def Run(self):
		if self._index < 0:
			if self._debug_gui:
				self.append(self._error_process, self._error_draw, self._error_evt,
					[None, None], "debug")
			self._index = 0
			self._evt()
		else:
			raise ValueError("process has already started")

	def _process(self):
		func = self._beltlinks[self._index][0]
		if callable(func):
			return func(self)
		return None

	def _draw(self):
		self._beltlinks[self._index][1](self)
		
	def _evt(self, val=None):
		retval = self._beltlinks[self._index][2](self, val)
		if retval is False:
			self.debug("Import aborted")
			Draw.Exit()
		elif retval is True:
			Draw.Exit()
			beltlen = len(self._beltlinks)
			while retval is True:
				self._volatile_data = dict()
				self._index += 1
				self._skipui = False
				if self._index >= beltlen: return
				try:
					retval = self._process()
				except FatalError:
					if not self.jumpToNamedLink('debug'): self._index = beltlen
					continue
				if retval is None and self._beltlinks[self._index][1] and \
					not self._skipui:
					if self._beltlinks[self._index][3]:
						input_handler = self._handle_mousewheel
					else:
						input_handler = None
					Draw.Register(self._draw, input_handler, self._evt)
					break

	def _handle_mousewheel(self, evt, val):
		offset = 0
		if evt is Draw.WHEELDOWNMOUSE: offset = 1
		elif evt is Draw.WHEELUPMOUSE: offset = -1
		if offset:
			scroll = self.getData('scroll-position', 0)
			scroll += offset
			min, max = self._beltlinks[self._index][3]
			if (min is None or scroll >= min) and (max is None or scroll <= max):
				self.setData('scroll-position', scroll)
				Draw.Redraw()
	
	def setData(self, key, value, persist=False):
		if persist: self._persistent_data[key] = value
		else: self._volatile_data[key] = value
	
	def getData(self, key, default=None):
		if key in self._volatile_data:
			return self._volatile_data[key]
		if key in self._persistent_data:
			return self._persistent_data[key]
		return default

	def __contains__(key):
		return key in self._volatile_data or key in self._persistent_data

	def _error_process(self, shim):
		if self._error_list:
			numerrors = len(self._error_list)
			rows = min(numerrors, 20)
			self.setData('num-errors', numerrors)
			self.setData('rows', rows)
			self.setScrollRange(0, numerrors - rows)
		else:
			self.skipUI()

	def _error_draw(self, shim):
		Draw.PushButton("Save log", 1, 31, 10, 140, 30, "Save the log to a file, and then continue with loading")
		Draw.PushButton("Finish", 0, 289, 10, 140, 30, "Do not save the errors, and continue with loading")
		BGL.glClearColor(0.7, 0.7, 0.7, 1)
		BGL.glClear(BGL.GL_COLOR_BUFFER_BIT)
		BGL.glColor3f(0.8, 0.8, 0.8)
		BGL.glRecti(5, 5, 455, 405)
		BGL.glColor3f(0.7, 0.7, 0.7)
		BGL.glRecti(10, 45, 450, 365)
		BGL.glRecti(175, 15, 285, 35)
		BGL.glRecti(15, 370, 440, 400)
		rows, scroll = self.getData('rows'), self.getData('scroll-position', 0)
		numerrors = self.getData('num-errors')
		Draw.Label("%i-%i of %i" % (scroll, scroll + rows, numerrors),
			176, 16, 108, 18)
		start = scroll
		end = start + rows
		BGL.glColor3i(0, 0, 0)
		counter = 0
		while start + counter < end:
			BGL.glRasterPos2i(11, 355 - counter * 16)
			Draw.Text(self._error_list[start + counter])
			counter += 1
		BGL.glColor3i(0, 0, 0)
		text = ("Processing Errors", "large")
		BGL.glRasterPos2i(int((426 - Draw.GetStringWidth(*text)) / 2 + 15), 381)
		Draw.Text(*text)
	
	def _error_evt(self, shim, val):
		if val == 0:
			return True
		if val == 1:
			filename = self.getData('filename').rsplit('.', 1)[0] + ".log"
			Window.FileSelector(self._dump_log, 'Save import log', filename)
			return
		errpage = self.getData('error-page') 
		if val == 2: self.setData('error-page', errpage - 1)
		elif val == 3: self.setData('error-page', errpage + 1)
		Draw.Redraw()

	def _dump_log(self, filename):
		f = open(filename, "w")
		f.write(self._debug_buffer)
		f.close()

def normalizeObjectName(name):
	pos = name.rfind('.')
	if name[pos + 1:].isdigit(): name = name[:pos]
	return name

def validate(levels):
	""" expects levels pre-sorted by object name, and assumes that all passed
	    objects have names starting with LEVEL_ and are children of a PIE_ object
	"""
	errors = list()
	if not levels:
		errors.append(('error', "no levels were found"))
	i = 0
	for level in levels:
		i += 1
		name = level.getName()
		invalid = True
		try:
			listed_num = int(normalizeObjectName(name)[6:])
			if listed_num is i: invalid = False
		except ValueError:
			pass
		if invalid:
			errors.append(('warning', "expected level %i" % i))
			i += 1
		if level.getType() != "Mesh":
			errors.append(('error', "%s must be a mesh object" % name))
			continue
		mesh = level.getData(mesh=True)
		num = len(mesh.faces)
		if num > 512:
			errors.append(('error', "more than 512 faces found in %s (%i found)" %
				(name, num)))
		num = len(mesh.verts)
		if num > 768:
			errors.append(('error', "more than 768 verts found in %s (%i found)" %
				(name, num)))
		for j, face in enumerate(mesh.faces):
			num = len(face.verts)
			if num > 6:
				errors.append(('error',	"more than 6 verts found in face %i of %s (%i found)" %
					(j, name, num)))
	return errors
