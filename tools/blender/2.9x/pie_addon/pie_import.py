#!/usr/bin/python3
# -*- coding: utf-8 -*- #

# ##### BEGIN GPL LICENSE BLOCK #####
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

import bpy
import bmesh
import os
from .shared import getTexAnimGrp


def convertStrListToFloatList(ls):
    return [float(word) for word in ls]

def convertStrListToIntList(ls):
    return [int(word) for word in ls]

def convertPolygonList(ls):
    """First 5 elements are definitely int,
    following elements are actually floats
    """
    out = [int(word) for word in ls[:5]]
    for i in range(5, len(ls)):
        out.append(float(ls[i]))
    return out

def newTexAnimGroup(ob, poly):
    ob.pie_tex_anim_grps.add()
    ob.pie_tex_anim_grp_index = len(ob.pie_tex_anim_grps) - 1
    grpII = ob.pie_tex_anim_grp_index
    grpName = 'Texture Animation Group {n}'.format(n=grpII + 1)

    ob.pie_tex_anim_grps[grpII].name = grpName
    ob.pie_tex_anim_grps[grpII].texAnimImages = int(poly[5])
    ob.pie_tex_anim_grps[grpII].texAnimRate = int(poly[6])
    ob.pie_tex_anim_grps[grpII].texAnimWidth = int(poly[7])
    ob.pie_tex_anim_grps[grpII].texAnimHeight = int(poly[8])


class Importer():

    headers = [
        'PIE', 'TYPE', 'TEXTURE', 'NORMALMAP', 'SPECULARMAP', 'EVENT',
        'LEVELS', 'LEVEL', 'POINTS', 'NORMALS', 'POLYGONS', 'CONNECTORS',
        'ANIMOBJECT', 'SHADOWPOINTS', 'SHADOWPOLYGONS',
    ]

    def pie_parse(self, pieFile):
        pie = open(pieFile, 'r')

        pie_info = {
            'PIE': 0,
            'TYPE': 0,
            'TEXTURE': '',
            'NORMALMAP': '',
            'SPECULARMAP': '',
            'EVENT': [],
            'LEVELS': [],
        }

        lvl = -1
        reading = ''

        for line in pie:

            if len(line) <= 1:
                continue

            ls = line.split()
            fl = ls[0]

            if fl in self.headers:
                reading = fl

                if fl in ['PIE', 'TYPE']:
                    pie_info[fl] = ls[1]

                elif fl in ['TEXTURE', 'NORMALMAP', 'SPECULARMAP']:
                    pie_info[fl] = ls[2]

                elif fl == 'EVENT':
                    pie_info[fl].append([ls[1], ls[2]])

                elif fl == 'LEVEL':
                    pie_info['LEVELS'].append({
                      'POINTS': [],
                      'NORMALS': [],
                      'POLYGONS': [],
                      'CONNECTORS': [],
                      'ANIMOBJECT': [],
                      'SHADOWPOINTS': [],
                      'SHADOWPOLYGONS': [],
                    })
                    lvl += 1

                elif fl == 'ANIMOBJECT':
                    pie_info['LEVELS'][lvl][fl].append(
                        ([int(ls[1]), int(ls[2])])
                    )

            else:

                if reading in ['POINTS', 'SHADOWPOINTS', 'CONNECTORS']:
                    li = convertStrListToFloatList(ls)

                    if reading == 'CONNECTORS':
                        result = (li[0] * 0.01, li[1] * 0.01, li[2] * 0.01)
                    else:
                        result = (li[0] * 0.01, li[2] * 0.01, li[1] * 0.01)
                elif reading == 'POLYGONS':
                    result = tuple(convertPolygonList(ls))

                elif reading in [
                    'NORMALS', 'ANIMOBJECT', 'SHADOWPOLYGONS'
                ]:
                    result = tuple(convertStrListToIntList(ls))

                pie_info['LEVELS'][lvl][reading].append(result)

        return pie_info

    def pie_generateBlenderObjects(self, pieParse, pieName):
        armature = bpy.data.armatures.new(pieName)
        armatureObject = bpy.data.objects.new(pieName, armature)
        bpy.context.collection.objects.link(armatureObject)

        armatureObject.pie_object_prop.pieType = 'ROOT'
        armatureObject.show_in_front = True

        currentLvl = 0

        for action in bpy.data.actions:
            if action.name == pieName + ' Anim':
                bpy.data.actions.remove(action)

        for level in pieParse['LEVELS']:
            currentLvl += 1

            nameStr = '{n} Level{ii}'.format(n=pieName, ii=currentLvl)

            bpy.context.view_layer.objects.active = armatureObject
            bpy.ops.object.mode_set(mode='EDIT', toggle=False)

            bone = armatureObject.data.edit_bones.new(nameStr)
            bone.head = (0, 0, 0)
            bone.tail = (0, 0.125, 0)

            boneName = bone.name

            bpy.ops.object.mode_set(mode='OBJECT', toggle=False)

            armatureObject.data.bones[boneName].use_relative_parent = True

            mesh = bpy.data.meshes.new(nameStr)
            mesh.uv_layers.new()

            meshOb = bpy.data.objects.new(nameStr, mesh)
            meshOb.parent = armatureObject
            meshOb.parent_type = 'BONE'
            meshOb.parent_bone = nameStr
            bpy.context.collection.objects.link(meshOb)

            meshOb.pie_object_prop.pieType = 'LEVEL'

            bpy.context.view_layer.objects.active = meshOb

            # * POINTS/POLYGONS * #

            pie_points = level['POINTS']
            pie_polygons = []

            for p in level['POLYGONS']:
                # Normals in Blender and in game are flipped
                # Import and export in opposite order to flip them
                pie_polygons.append((p[2], p[4], p[3]))
            mesh.from_pydata(pie_points, [], pie_polygons)

            animatedPolygons = []
            for ii, p in enumerate(level['POLYGONS']):
                L = ii * 3

                uvData = meshOb.data.uv_layers.active.data

                n = 256
                # m is for modifying the index taken as UV data
                # depending on if the PIE polygon is animated.
                m = 0
                if int(p[0] == 4200):
                    animatedPolygons.append(ii)
                    m = 4

                if pieParse['PIE'] == '3':
                    uvData[L + 0].uv = ((p[5 + m], -p[6 + m] + 1))
                    uvData[L + 1].uv = ((p[9 + m], -p[10 + m] + 1))
                    uvData[L + 2].uv = ((p[7 + m], -p[8 + m] + 1))
                elif pieParse['PIE'] == '2':
                    uvData[L + 0].uv = ((p[5 + m] / n, (-p[6 + m] / n) + 1))
                    uvData[L + 1].uv = ((p[9 + m] / n, (-p[10 + m] / n) + 1))
                    uvData[L + 2].uv = ((p[7 + m] / n, (-p[8 + m] / n) + 1))

            bpy.ops.object.mode_set(mode='EDIT', toggle=False)
            bm = bmesh.from_edit_mesh(mesh)
            faces = bm.faces

            for ii in animatedPolygons:
                p = level['POLYGONS'][ii]

                if meshOb.pie_tex_anim_grps is None:
                    newTexAnimGroup(meshOb, p)
                    layer = getTexAnimGrp(
                        bm, str(len(meshOb.pie_tex_anim_grps) - 1)
                    )
                    faces.ensure_lookup_table()
                    faces[ii][layer] = 1
                else:
                    success = False
                    for jj, tAnimGp in enumerate(meshOb.pie_tex_anim_grps):

                        if (int(p[5]) == tAnimGp.texAnimImages and
                                int(p[6]) == tAnimGp.texAnimRate and
                                int(p[7]) == tAnimGp.texAnimWidth and
                                int(p[8]) == tAnimGp.texAnimHeight):

                            layer = getTexAnimGrp(bm, str(jj))
                            # faces.ensure_lookup_table()
                            faces[ii][layer] = 1
                            success = True
                            break

                    if success is False:
                        newTexAnimGroup(meshOb, p)
                        layer = getTexAnimGrp(
                            bm, str(len(meshOb.pie_tex_anim_grps) - 1)
                        )
                        faces.ensure_lookup_table()
                        faces[ii][layer] = 1

            bpy.ops.object.mode_set(mode='OBJECT', toggle=False)

            # * SHADOW POINTS/POLYGONS * #

            if level['SHADOWPOINTS'] and level['SHADOWPOLYGONS']:

                meshOb.pie_object_prop.shadowType = 'CUSTOM'

                shMesh = bpy.data.meshes.new(nameStr + ' Shadow')
                shMeshOb = bpy.data.objects.new(nameStr + ' Shadow', shMesh)
                shMeshOb.parent = meshOb

                shMeshOb.pie_object_prop.pieType = 'SHADOW'

                bpy.context.collection.objects.link(shMeshOb)

                sh_points = level['SHADOWPOINTS']
                sh_polygons = []

                for p in level['SHADOWPOLYGONS']:
                    sh_polygons.append((p[2], p[3], p[4]))

                shMesh.from_pydata(sh_points, [], sh_polygons)

            # * CONNECTORS * #

            ii = 1
            for connector in level['CONNECTORS']:
                connectorOb = bpy.data.objects.new(
                    '{name} Connector{num}'.format(name=nameStr, num=ii), None
                )
                connectorOb.empty_display_size = 0.125
                connectorOb.empty_display_type = 'ARROWS'
                connectorOb.location = connector
                connectorOb.parent = meshOb

                connectorOb.pie_object_prop.pieType = 'CONNECTOR'

                bpy.context.collection.objects.link(connectorOb)

                ii += 1

            # * ANIMOBJECT * #

            bpy.context.view_layer.objects.active = armatureObject
            bpy.ops.object.mode_set(mode='POSE', toggle=False)

            poseBone = armatureObject.pose.bones[boneName]

            if level['ANIMOBJECT']:

                if armatureObject.animation_data is None:
                    armatureObject.animation_data_create()
                    action = bpy.data.actions.new(pieName + ' Anim')

                armatureObject.animation_data.action = action

                lAP = 'pose.bones["{n}"].location'.format(n=nameStr)
                rAP = 'pose.bones["{n}"].rotation_euler'.format(n=nameStr)
                sAP = 'pose.bones["{n}"].scale'.format(n=nameStr)

                lXCrv = action.fcurves.new(lAP, index=0, action_group=nameStr)
                lYCrv = action.fcurves.new(lAP, index=1, action_group=nameStr)
                lZCrv = action.fcurves.new(lAP, index=2, action_group=nameStr)
                rXCrv = action.fcurves.new(rAP, index=0, action_group=nameStr)
                rYCrv = action.fcurves.new(rAP, index=1, action_group=nameStr)
                rZCrv = action.fcurves.new(rAP, index=2, action_group=nameStr)
                sXCrv = action.fcurves.new(sAP, index=0, action_group=nameStr)
                sYCrv = action.fcurves.new(sAP, index=1, action_group=nameStr)
                sZCrv = action.fcurves.new(sAP, index=2, action_group=nameStr)

                if len(level['ANIMOBJECT']) > 1:
                    self.scene.frame_start = level['ANIMOBJECT'][1][0]

                for key in level['ANIMOBJECT']:
                    if len(key) < 10:
                        meshOb.pie_object_prop.animTime = key[0]
                        meshOb.pie_object_prop.animCycle = key[1]
                        continue

                    lXCrv.keyframe_points.insert(key[0], key[1] * 0.00001)
                    lYCrv.keyframe_points.insert(key[0], key[2] * 0.00001)
                    lZCrv.keyframe_points.insert(key[0], key[3] * 0.00001)
                    rXCrv.keyframe_points.insert(key[0], key[4] * 0.0000174533)
                    rYCrv.keyframe_points.insert(key[0], key[5] * 0.0000174533)
                    rZCrv.keyframe_points.insert(key[0], key[6] * 0.0000174533)
                    sXCrv.keyframe_points.insert(key[0], key[7])
                    sYCrv.keyframe_points.insert(key[0], key[8])
                    sZCrv.keyframe_points.insert(key[0], key[9])

                    if self.scene.frame_end < key[0]:
                        self.scene.frame_end = key[0]

            armatureObject.select_set(True)
            bpy.context.view_layer.objects.active = armatureObject

            poseBone.rotation_mode = 'XYZ'

            bpy.ops.object.mode_set(mode='OBJECT', toggle=False)

            self.pie_loadProperties(pieParse, armatureObject)

    def pie_loadProperties(self, pieParse, pieObject):
        objProp = pieObject.pie_object_prop

        objProp.pieVersion = pieParse['PIE']
        objProp.texture = pieParse['TEXTURE']
        objProp.normal = pieParse['NORMALMAP']
        objProp.specular = pieParse['SPECULARMAP']

        for event in pieParse['EVENT']:
            if event[0] == '1':
                objProp.event1 = event[1]
            elif event[0] == '2':
                objProp.event2 = event[1]
            elif event[0] == '3':
                objProp.event3 = event[1]

        def getMaskArray(pieType, len):
            flagsStr = str(bin(int(pieType, 16)))[2:][::-1].ljust(len, '0')
            return [int(ii) for ii in flagsStr]

        flags = getMaskArray(pieParse['TYPE'], 17)

        objProp.adrOff = flags[0]
        objProp.adrOn = flags[1]
        objProp.pmr = flags[2]
        objProp.pitch = flags[4]
        objProp.roll = flags[5]
        objProp.reserved = flags[9]
        objProp.stretch = flags[12]
        objProp.tcMask = flags[16]

    def pie_import_quick(self, scene, pieDir, pieMesh):
        self.scene = scene

        pieParse = self.pie_parse(os.path.join(pieDir,pieMesh))

        self.pie_generateBlenderObjects(pieParse, pieMesh)

    def pie_import(self, scene, pieDir):
        self.scene = scene

        pieParse = self.pie_parse(pieDir)

        self.pie_generateBlenderObjects(
            pieParse, os.path.splitext(os.path.basename(pieDir))[0]
        )
