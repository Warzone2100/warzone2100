#!/usr/bin/env python

""" Provides a serial parser and support functions for PIE models

All parser events will be passed back as a list starting with the event
type followed by the line number in the input file. Type-specific data
starts at position 3 in the list.

All directives will have the directive name (case preserved) and the
integer field following it in the list.

TEXTURE and NOTEXTURE directives will also have a filename string followed
by two integers (texture dimensions) appended to the list.

Anything not determined to be a directive is "data" whose components will
be left as strings unless passed through data_mutator, with the exception
of BSP data, which will remain unprocessed unless passed through
bsp_mutator.

Events of type "error" will always have an instance of a PIEParseError
derived class and possibly the type of expected token appended to the list.

Instances of PIESyntaxError indicate that no data from that line can be
trusted. Conversely, instances of PIEStructuralError indicate that the
minimum expected data was present (excess data may be appended to
the yielded list), but, depending on the context, data found on other
lines may not be trustworthy, such as in the hypothetical case of a
'LEVEL 5' directive appearing before a 'LEVEL 2' directive.

When a PIEStructuralError is passed through the generator, the proceeding
token will be the normal parsed contents of the same line, however a
PIEStructuralError instance will never be followed by data from the same
line to which the error applies.

"""

__version__ = "1.0"
__author__ = "Kevin Gillette"

# --------------------------------------------------------------------------
# pie v1.0 by Kevin Gillette (kage)
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

TOKEN_TYPE = 0
LINENO = 1
LINE = 2
FIRST = 3 # use for retrieving data from polygons, points, connectors, etc.
DIRECTIVE_NAME = 3
DIRECTIVE_VALUE = 4
TEXTURE_FILENAME = 5
TEXTURE_WIDTH, TEXTURE_HEIGHT = 6, 7
ERROR = 3
ERROR_ASSUMED_TYPE = 4 # if it weren't an error, what would it be processed as?

class PIEParseError(Exception):
  """ Base class for all PIE-related parsing errors """

class PIESyntaxError(PIEParseError):
  """ Raised for line-fatal parsing errors """

class PIEStructuralError(PIEParseError):
  """ Raised for non-fatal parsing errors related to the PIE specification """

def _handle_texture(s):
  """ Breaks a 'TEXTURE' directive into its component parts. """

  type, s = s.split(None, 1)
  s, x, y = s.rsplit(None, 2)
  try:
    return [int(type), s, int(x), int(y)]
  except ValueError:
    raise PIESyntaxError("expected integer")

def _handle_type(s):
  try:
    return [int(s, 16)]
  except ValueError:
    raise PIESyntaxError("expected a hexadecimal integer")

_directive_handlers = {
  'TEXTURE': _handle_texture,
  'NOTEXTURE': _handle_texture,
  'TYPE': _handle_type
}

def parse(pie):
  """ Parses non-binary PIE files and yields lists of tokens via a generator
  
  "pie" may be a file-like object, or a filename string.

  Uses Python's universal EOL support. Makes the assumption that tokens
  may not span multiple lines, which differs from Warzone 2100's internal
  parser. Directives such as "PIE" or "CONNECTORS" have the proceeding
  tokens converted into python integers, with the except of "TEXTURE" and
  "NOTEXTURE", which contain extra data within the declaration.
  
  """

  if isinstance(pie, basestring):
    pie = open(pie, "rU")
  elif pie.closed:
    raise TypeError("parse() takes either an open filehandle or a filename")

  lineno = 0
  for line in pie:
    lineno += 1
    vals = [None, lineno, line]
    rest = line.split(None, 1)
    if not rest: continue # blank line
    first = rest[0]
    if first.isalpha() and first[0].isupper():
      if len(rest) == 1:
        vals[0] = "error"
        vals[3:] = [PIESyntaxError("expected more data"), "directive"]
        yield vals
        continue
      vals[0] = "directive"
      vals.append(first)
      if first in _directive_handlers:
        try:
          vals.extend(_directive_handlers[first](rest[1]))
        except PIEParseError, instance:
          vals[3:] = [instance, "directive"]
          vals[0] = "error"
      else:
        rest = rest[1].split()
        try:
          vals.append(int(rest[0]))
        except ValueError:
          vals[3:] = [PIESyntaxError("integer expected"), "directive"]
          vals[0] = "error"
        if len(rest) > 1:
          yield ["error", lineno, line, PIEStructuralError(
            "unexpected additional data")]
          vals.extend(rest[1:])
    else:
      vals[0] = "data"
      vals.append(first)
      try:
        vals.extend(rest[1].split())
      except IndexError:
        yield ["error", lineno, line, PIESyntaxError("malformed line")]
    yield vals

def data_mutator(gen):
  """ Modifies "data" tokens.

  Converts strings to ints or floats depending on the most recent
  directive. Validates "data" tokens where convenient.

  Data following a POINTS directive will always have its components
  converted to floats.

  Data following a POLYGONS directive will have the first two fields
  converted to integers, followed by a x number of ints and the rest
  as floats where x is the value of the second field.

  """

  mode = 0
  for i in gen:
    ilen = len(i)
    if "directive" == i[0]:
      directive = i[3]
      if "POINTS" == directive: mode, name = 1, "point"
      elif "CONNECTORS" == directive: mode, name = 1, "connector"
      elif "POLYGONS" == directive: mode = 2
      else: mode = 0
    elif "data" != i[0] or 0 == mode: pass
    elif 1 == mode: # points
      if ilen == 6:
        i[0] = name
        try:
          i[3:] = map(float, i[3:])
        except ValueError:
          i[0] = "error"
          i[3:] = [PIESyntaxError("expected a floating-point number"), name]
      else:
        i[0] = "error"
        i[3:] = [PIESyntaxError("not a valid " + name), name]
    elif 2 == mode: # polygons
      valid = False
      if ilen > 7:
        try:
          type, points = int(i[3], 16), int(i[4])
          if ilen > 4 + points and (ilen - 5 - points) % 2 == 0:
            i[0], i[3], i[4], pos = "polygon", type, points, 5 + points
            i[5:pos] = point_list = map(int, i[5:pos])
            i[pos:] = map(float, i[pos:])
            for pos in xrange(points - 1):
              if point_list.count(point_list[pos]) > 1:
                yield ["error", i[1], i[2], PIEStructuralError(
                  "duplicate vertices on same polygon")]
                break
            valid = True
        except ValueError:
          i[0] = "error"
          i[3:] = [PIESyntaxError("expected a number"), "polygon"]
          valid = True
      if not valid:
        i[0] = "error"
        i[3:] = [PIESyntaxError("not a valid polygon"), "polygon"]
    yield i

def bsp_mutator(gen):
  mode = 0
  for i in gen:
    if "directive" == i[0]:
      if "BSP" == i[3]: mode = 1
      else: mode = 0
    elif "data" != i[0]: pass
    elif 1 == mode:
      try:
        i[3:] = map(int, i[3:])
        i[0] = "bsp-data"
      except ValueError:
        i[3:] = [PIESyntaxError("expected an integer"), "bsp-data"]
        i[0] = "error"
    yield i

if __name__ == "__main__":
  import sys
  args = sys.argv
  mutate = True
  if "--no-mutate" in args:
    del args[args.index("--no-mutate")]
    mutate = False
  if len(args) < 2:
    sys.exit("when run directly, a filename argument is required")
  filename = args[1]
  gen = parse(filename)
  if mutate: gen = bsp_mutator(data_mutator(gen))
  for i in gen:
      print i,
      if i[0] == "error":
        print i[3],
      print

# Setup VIM: ex: et ts=2
