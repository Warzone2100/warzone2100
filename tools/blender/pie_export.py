#!BPY
 
"""
Name: 'Warzone model (.pie)...'
Blender: 244
Group: 'Export'
Tooltip: 'Save a Warzone model file'
"""

__author__ = "Gerard Krol"
__version__ = "0.1"
__bpydoc__ = """\
This script exports Warzone 2100 PIE files.
"""

#
# --------------------------------------------------------------------------
# PIE Export v0.1 by Gerard Krol (gerard_)
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

def fs_callback(filepath):
		# TODO: export all meshes instead of only the selected one
		ob = Blender.Object.GetSelected()
		mesh = ob[0].getData(mesh=1)
		
		out = file(filepath, 'w')
		out.write( 'PIE 2\n' )
		out.write( 'TYPE 200\n' )
		#print mesh.faces[0].image.getFilename()
		texturename = os.path.basename(mesh.faces[0].image.getFilename())
		print 'texture:', texturename
		out.write( 'TEXTURE 0 ' + texturename + ' 256 256\n')
		out.write( 'LEVELS 1\n')
		out.write( 'LEVEL 1\n')
		out.write('POINTS ' + str(len(mesh.verts)) + '\n')

		for vert in mesh.verts:
				out.write( '\t%i %i %i\n' % (vert.co.x*128, vert.co.z*128, -vert.co.y*128) )
		
		out.write('POLYGONS ' + str(len(mesh.faces)) + '\n')
		uvLayers = mesh.getUVLayerNames()
		for face in mesh.faces:
			flags = 0x200
			if len(uvLayers) > 1:
				# team colored
				mesh.activeUVLayer = uvLayers[0]
				orgUVCoords = []
				for n in xrange(0, len(face.v)):
					orgUVCoords.append((face.uv[n][0], 1.0-face.uv[n][1]) )
				mesh.activeUVLayer = uvLayers[1]
				if face.mode & Blender.Mesh.FaceModes['TEX']:
					uDiff = 0.0
					vDiff = 0.0
					for n in xrange(0, len(face.v)):
						uDiff += face.uv[n][0] - orgUVCoords[n][0]
						vDiff += 1.0-face.uv[n][1] - orgUVCoords[n][1]
					uDiff /= len(face.v)
					vDiff /= len(face.v)
					print uDiff, vDiff
					if abs(uDiff) > abs(vDiff):
						width = uDiff*256/7
						height = 256
					else:
						width = 256
						height = vDiff*256/7
					flags |= 0x4000
			if face.mode & Blender.Mesh.FaceModes['TWOSIDE']:
				flags |= 0x2000
			if face.transp == Blender.Mesh.FaceTranspModes['ALPHA']:
				flags |= 0x800
				
			mesh.activeUVLayer = uvLayers[0]
			out.write('\t' + hex(flags).split('x')[-1] + ' ' + str(len(face.v)))
			
			for vert in face.v:
					out.write( ' %i' % vert.index )
			if flags & 0x4000:
				# team colors
				out.write( ' 8 1 %i %i' % (width, height) )
			for n, vert in enumerate(face.v):
					#print face.uv[n]
					out.write( ' %i %i' % (face.uv[n][0]*256, (1.0-face.uv[n][1])*256) )
			out.write('\n')
		# export the connectors
		connectors = []
		for object in Blender.Scene.GetCurrent().getChildren():
			if object.getData() == None:
				x,y,z = object.getLocation()
				connectors.append((object.getName(), (x*128, -y*128 ,z*128)))
		connectors.sort()
		if connectors:
			out.write('CONNECTORS %i\n' % len(connectors))
			for c, xyz in connectors:
				out.write('\t%i %i %i\n' % xyz)
		out.close()
fname = Blender.sys.makename(ext=".pie")
Blender.Window.FileSelector(fs_callback, "Export Warzone Model", fname)
