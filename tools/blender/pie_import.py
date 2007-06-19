#!BPY
 
"""
Name: 'Warzone model (.pie)...'
Blender: 244
Group: 'Import'
Tooltip: 'Load a Warzone model file'
"""

__author__ = "Rodolphe Suescun, Gerard Krol, Kevin Gillette"
__url__ = ["blender"]
__version__ = "0.2.1"

__bpydoc__ = """\
This script imports PIE files to Blender.

Usage:

Run this script from "File->Import" menu and then load the desired PIE file.
"""

#
# --------------------------------------------------------------------------
# PIE Import v0.1 by Rodolphe Suescun (AKA RodZilla)
#            v0.2 by Gerard Krol (gerard_)
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
import os

#==================================================================================#
# This loads data from .pie file                                                   #
#==================================================================================#
def load_pie(file):
	time1 = sys.time()

	DIR = os.path.dirname(file)

	fileLines = open(file, 'r').readlines()

	#==================================================================================#
	# Make split lines, ignore blenk lines or comments.                                #
	#==================================================================================#
	lIdx = 0 
	while lIdx < len(fileLines):
		fileLines[lIdx] = fileLines[lIdx].split()
		lIdx+=1

	lIdx = 0
	pieVersion = -1
	image = None
	while lIdx < len(fileLines):
		l = fileLines[lIdx]
		if len(l) == 0:
			fileLines.pop(lIdx)
	
		elif l[0] == 'PIE':
			pieVersion = int(l[1])
			fileLines.pop(lIdx)
			print 'PIE version ', pieVersion

		elif l[0] == 'TEXTURE':
			currentMat = Material.New('PIE_mat')
			loaded = 1
			
			# the big image finding trick
			basename, ext = os.path.splitext(' '.join(l[2:-2]).lower())
			namelen = len(basename) + len(ext)

			print 'basename:', basename
			print 'ext:', ext
			options = []
			for d in (('..', 'texpages'), ('..', '..', 'texpages')):
				file_found = False
				d = os.path.join(DIR, *d)
				if not os.path.exists(d): continue
				for fn in os.listdir(d):
					if fn.startswith(basename):
						canonical = os.path.abspath(os.path.join(d, fn))
						if fn.endswith(ext) and len(fn) == namelen:
							options.insert(0, canonical)
							file_found = True
							break
						else:
							options.append(canonical)
				if file_found: break

			print 'options:', options
			image = Image.Load(options[0])

			texture = Texture.New()
			texture.setType('Image')
			texture.image = image
			currentMat.setTexture(0, texture, Texture.TexCo.UV, Texture.MapTo.COL)
			fileLines.pop(lIdx)
				
		elif l[0] == 'LEVEL':
			mesh = Mesh.New('PIE')
			mesh.materials.append(currentMat)
			mesh.addUVLayer('base') 
			mesh.addUVLayer('teamcolors') 
			fileLines.pop(lIdx)

		elif l[0] == 'POINTS':
			nbPoints = int(l[1])
			fileLines.pop(lIdx)
			point = 0
			while point < nbPoints:
				l = fileLines[lIdx]
				#todo: convert to Mesh code
				mesh.verts.extend(float(l[0])/128.0, -float(l[2])/128.0, float(l[1])/128.0)
				fileLines.pop(lIdx)
				point+=1
				
		elif l[0] == 'POLYGONS':
			nbPolys = int(l[1])
			fileLines.pop(lIdx)
			for poly in xrange(0,nbPolys):
				l = fileLines[lIdx]
				flags = int(l[0],16)
				nbPoints = int(l[1])
				#f=Mesh.MFace()
				#f.mat = 0
				# set up verticles
				vertsInFace = []
				for point in xrange(0, nbPoints):
					vertsInFace.append(int(l[point+2]))
				#print vertsInFace
				mesh.faces.extend(vertsInFace, ignoreDups=True)
				#print poly, len(mesh.faces)
				f = mesh.faces[poly]
				#print 'face', f
				f.mat = 0
				# set up uv coords
				mesh.activeUVLayer = 'base'
				f.image=image
				uvCoords = []
				for point in xrange(0, nbPoints):
					uvCoords.append(Mathutils.Vector(float(l[point*2-nbPoints*2])/256.0, 1.0-float(l[point*2-nbPoints*2+1])/256.0))
				print 'UVs', uvCoords
				f.uv = uvCoords
				# set up uv coords
				if flags & 0x4000:
					mesh.activeUVLayer = 'teamcolors'
					f.image=image
					uvCoords = []
					max = int(l[1+nbPoints])
					time = int(l[1+nbPoints+1])
					width = int(l[1+nbPoints+2])
					height = int(l[1+nbPoints+3])
					print 'max', max
					print 'time', time
					print 'width', width
					print 'height', height
					offsetU = height*7
					offsetV = 0
					while offsetU > 255:
						offsetU -= 256
						offsetV += width
					print 'offset', offsetU, offsetV
					for point in xrange(0, nbPoints):
						# team colors
						uvCoords.append(Mathutils.Vector(float(l[point*2-nbPoints*2])/256.0+float(offsetU)/256.0, 1.0-float(l[point*2-nbPoints*2+1])/256.0-float(offsetV)/256.0))
					f.uv = uvCoords
				
				
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
				#mesh.faces.append(f)
				fileLines.pop(lIdx)
				#poly+=1
		elif l[0] == 'CONNECTORS':
			nbConnectors = int(l[1])
			fileLines.pop(lIdx)
			connector = 0
			# we only load three... the rest is junk
			while connector < 3 and connector < nbConnectors:
				l = fileLines[lIdx]
				x = float(l[0])/128.0
				y = -float(l[1])/128.0
				z = float(l[2])/128.0
				scene = Scene.GetCurrent()      # get the current scene
				empty = Object.New('Empty', 'Connector')

				# convert our number strings to floats
				empty.loc = x, y, z
				empty.setSize(0.15, 0.15, 0.15)
				scene.link(empty)   # link our new object into scene
	
				#empty = Object.New('Empty')
				#ob = scn.objects.new(empty, 'Empty')           # make a new object in this scene using the camera data
				#ob.setLocation (x, y, z)       # position the object in the scene
				fileLines.pop(lIdx)
				connector+=1
			while connector < nbConnectors:
				fileLines.pop(lIdx)
				
		else:
			lIdx+=1

	#NMesh.PutRaw(mesh, "pie", 1)
	mesh.activeUVLayer = 'base'
	scn = Scene.GetCurrent()          # link object to current scene
	ob = scn.objects.new(mesh, 'PIE')
	Redraw()
	
	print "pie import time: ", sys.time() - time1

Window.FileSelector(load_pie, 'Import Warzone model')

