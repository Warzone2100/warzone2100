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


def getNumbers(string):
    numbers = []
    for word in string.split():
        if word.isdigit():
            numbers.append(int(word))

    return numbers

def convertStrListToNumList(ls):
    val = []

    for word in ls:
        val.append(float(word))

    return val

def newTexAnimGroup(ob, polygon, index):
    ob.pie_texture_animation_groups.add()
    ob.pie_texture_animation_group_index = len(ob.pie_texture_animation_groups) - 1
    texAnimGroupindex = ob.pie_texture_animation_group_index

    ob.pie_texture_animation_groups[texAnimGroupindex].name = 'Texture Animation Group {num}'.format(num=texAnimGroupindex + 1)
    ob.pie_texture_animation_groups[texAnimGroupindex].texAnimImages = int(polygon[5])
    ob.pie_texture_animation_groups[texAnimGroupindex].texAnimRate = int(polygon[6])
    ob.pie_texture_animation_groups[texAnimGroupindex].texAnimWidth = int(polygon[7])
    ob.pie_texture_animation_groups[texAnimGroupindex].texAnimHeight = int(polygon[8])

    ob.pie_texture_animation_groups[texAnimGroupindex].texAnimFaces += ' {str} '.format(str=str(index))


class Importer():

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

        currentLevel = -1
        currentlyReading = ''

        for line in pie:

            if len(line) <= 1:
                    continue

            if 'PIE' in line:
                currentlyReading = 'PIE'
                pie_info['PIE'] = getNumbers(line)[0]
                continue

            elif 'TYPE' in line:
                currentlyReading = 'TYPE'
                pie_info['TYPE'] = getNumbers(line)[0]
                continue

            elif 'TEXTURE' in line:
                currentlyReading = 'TEXTURE'
                pie_info['TEXTURE'] = line.split()[2]
                continue

            elif 'NORMALMAP' in line:
                currentlyReading = 'NORMALMAP'
                pie_info['NORMALMAP'] = line.split()[2]
                continue

            elif 'SPECULARMAP' in line:
                currentlyReading = 'SPECULARMAP'
                pie_info['SPECULARMAP'] = line.split()[2]
                continue

            elif 'EVENT' in line:
                currentlyReading = 'EVENT'
                ls = line.split()
                pie_info['EVENT'].append([ls[1], ls[2]])
                continue

            elif 'LEVELS' in line:
                currentlyReading = 'LEVELS'
                continue

            elif 'LEVEL' in line:
                currentlyReading = 'LEVEL'
                pie_info['LEVELS'].append({
                'POINTS': [],
                'NORMALS': [],
                'POLYGONS': [],
                'CONNECTORS': [],
                'ANIMOBJECT': [],
                })
                currentLevel += 1
                continue

            elif 'POINTS' in line:
                currentlyReading = 'POINTS'
                continue

            elif 'NORMALS' in line:
                currentlyReading = 'NORMALS'
                continue

            elif 'POLYGONS' in line:
                currentlyReading = 'POLYGONS'
                continue

            elif 'CONNECTORS' in line:
                currentlyReading = 'CONNECTORS'
                continue

            elif 'ANIMOBJECT' in line:
                currentlyReading = 'ANIMOBJECT'
                ls = line.split()
                pie_info['LEVELS'][currentLevel]['ANIMOBJECT'].append(tuple([float(ls[1]), float(ls[2])]))
                continue

            else:

                if currentlyReading is 'POINTS':
                    l = convertStrListToNumList(line.split())
                    v = (l[0] * 0.01, l[2] * 0.01, l[1] * 0.01)

                    pie_info['LEVELS'][currentLevel]['POINTS'].append(v)
                    continue

                elif currentlyReading is 'NORMALS':
                    pie_info['LEVELS'][currentLevel]['NORMALS'].append(tuple(convertStrListToNumList(line.split())))
                    continue

                elif currentlyReading is 'POLYGONS':
                    pie_info['LEVELS'][currentLevel]['POLYGONS'].append(tuple(convertStrListToNumList(line.split())))
                    continue

                elif currentlyReading is 'CONNECTORS':
                    l = convertStrListToNumList(line.split())
                    v = (l[0] * 0.01, l[1] * 0.01, l[2] * 0.01)

                    pie_info['LEVELS'][currentLevel]['CONNECTORS'].append(v)
                    continue

                elif currentlyReading is 'ANIMOBJECT':
                    pie_info['LEVELS'][currentLevel]['ANIMOBJECT'].append(tuple(convertStrListToNumList(line.split())))
                    continue

        return pie_info

    def pie_generateBlenderObjects(self, pieParse):

        pieFile = self.scene.pie_import_prop.pieFile

        armature = bpy.data.armatures.new(pieFile)
        armatureObject = bpy.data.objects.new(pieFile, armature)
        bpy.context.collection.objects.link(armatureObject)

        armatureObject.pie_object_prop.pieType = 'ROOT'

        armatureObject.show_in_front = True

        currentLevel = 0
        
        for action in bpy.data.actions:
            if action.name == pieFile + ' Anim':
                bpy.data.actions.remove(action)

        for level in pieParse['LEVELS']:
            currentLevel += 1

            nameStr = '{name} Level{num}'.format(name=pieFile, num=currentLevel)

            bpy.context.view_layer.objects.active = armatureObject
            bpy.ops.object.mode_set(mode='EDIT', toggle=False)

            bone = armatureObject.data.edit_bones.new(nameStr)
            bone.head = (0, 0, 0)
            bone.tail = (0, 0.125, 0)
            
            boneName = bone.name

            bpy.ops.object.mode_set(mode='OBJECT', toggle=False)
            
            mesh = bpy.data.meshes.new(nameStr)
            meshObject = bpy.data.objects.new(nameStr , mesh)
            meshObject.parent = armatureObject
            meshObject.parent_type = 'BONE'
            meshObject.parent_bone = nameStr
            bpy.context.collection.objects.link(meshObject)

            meshObject.pie_object_prop.pieType = 'LEVEL'

            bpy.context.view_layer.objects.active = meshObject

            #* POINTS/POLYGONS *#

            pie_points = level['POINTS']
            pie_polygons = []
            
            for poly in level['POLYGONS']:
                pie_polygons.append((poly[2], poly[3], poly[4]))


            mesh.from_pydata(pie_points, [], pie_polygons)

            ii = 0
            for poly in level['POLYGONS']:
                loop = ii * 3

                if int(poly[0]) == 200:

                    if pieParse['PIE'] is 3:
                        meshObject.data.uv_layers.active.data[loop + 0].uv = ((poly[5], poly[6]))
                        meshObject.data.uv_layers.active.data[loop + 1].uv = ((poly[7], poly[8]))
                        meshObject.data.uv_layers.active.data[loop + 2].uv = ((poly[9], poly[10]))
                    elif pieParse['PIE'] is 2:
                        meshObject.data.uv_layers.active.data[loop + 0].uv = ((poly[5] / 256, poly[6] / 256))
                        meshObject.data.uv_layers.active.data[loop + 1].uv = ((poly[7] / 256, poly[8] / 256))
                        meshObject.data.uv_layers.active.data[loop + 2].uv = ((poly[9] / 256, poly[10] / 256))

                elif int(poly[0]) == 4200:

                    if meshObject.pie_texture_animation_groups is None:
                        newTexAnimGroup(meshObject, poly, ii)
                    else:
                        success = False
                        for texAnimGroup in meshObject.pie_texture_animation_groups:
                    
                            if (int(poly[5]) == texAnimGroup.texAnimImages and int(poly[6]) == texAnimGroup.texAnimRate and int(poly[7]) == texAnimGroup.texAnimWidth and int(poly[8]) == texAnimGroup.texAnimHeight):
                                texAnimGroup.texAnimFaces += ' {str} '.format(str=str(ii))
                                success = True
                                break
                    
                        if success is False:
                            newTexAnimGroup(meshObject, poly, ii)
                            

                    if pieParse['PIE'] is 3:
                        meshObject.data.uv_layers.active.data[loop + 0].uv = ((poly[9], poly[10]))
                        meshObject.data.uv_layers.active.data[loop + 1].uv = ((poly[11], poly[12]))
                        meshObject.data.uv_layers.active.data[loop + 2].uv = ((poly[13], poly[14]))
                    elif pieParse['PIE'] is 2:
                        meshObject.data.uv_layers.active.data[loop + 0].uv = ((poly[9] / 256, poly[10] / 256))
                        meshObject.data.uv_layers.active.data[loop + 1].uv = ((poly[11] / 256, poly[12] / 256))
                        meshObject.data.uv_layers.active.data[loop + 2].uv = ((poly[13] / 256, poly[14] / 256))
                    
                ii += 1

            #* NORMALS *#

            #if level['NORMALS']:
                #meshObject.pie_object_prop.exportNormals = True

                #normals = []
                #verts = []
                #ii = 0
                #for poly in mesh.polygons:
                #    jj = 0
                #    poly.use_smooth = True
                #    print(poly.normal)
                #    for vertex in poly.vertices:
                #        if vertex not in verts:
                #            verts.append(vertex)
                #            meshObject.data.vertices[vertex].normal = (level['NORMALS'][ii][jj], level['NORMALS'][ii][jj + 1], level['NORMALS'][ii][jj + 2])
                #            #print(*(level['NORMALS'][ii][jj], level['NORMALS'][ii][jj + 1], level['NORMALS'][ii][jj + 2]))
                #            #print(meshObject.data.vertices[vertex].normal)
                #            normals.append(meshObject.data.vertices[vertex].normal)
                #        jj += 3
                #    ii += 1

                #mesh.create_normals_split()

                #mesh.calc_normals_split()

                #mesh.show_normal_vertex = mesh.show_normal_loop = True

                #mesh.normals_split_custom_set([(0,0,0) for l in mesh.loops])
                #mesh.normals_split_custom_set_from_vertices(normals)

            #* CONNECTORS *#

            ii = 1
            for connector in level['CONNECTORS']:
                connectorObject = bpy.data.objects.new('{name} Connector{num}'.format(name=nameStr, num=ii), None)
                connectorObject.empty_display_size = 0.125
                connectorObject.empty_display_type = 'ARROWS'
                connectorObject.location = connector
                connectorObject.parent = meshObject

                connectorObject.pieType = 'CONNECTOR'

                self.scene.collection.objects.link(connectorObject)

                ii += 1

            #* ANIMOBJECT *#

            bpy.context.view_layer.objects.active = armatureObject
            bpy.ops.object.mode_set(mode='POSE', toggle=False)

            poseBone = armatureObject.pose.bones[boneName]

            if level['ANIMOBJECT']:

                if armatureObject.animation_data is None:
                    armatureObject.animation_data_create()
                    action = bpy.data.actions.new(pieFile + ' Anim')

                armatureObject.animation_data.action = action

                locAnimPath = 'pose.bones["{name}"].location'.format(name=nameStr)
                rotAnimPath = 'pose.bones["{name}"].rotation_euler'.format(name=nameStr)
                sclAnimPath = 'pose.bones["{name}"].scale'.format(name=nameStr)

                locXCurve = action.fcurves.new(locAnimPath, index=0, action_group=nameStr)
                locYCurve = action.fcurves.new(locAnimPath, index=1, action_group=nameStr)
                locZCurve = action.fcurves.new(locAnimPath, index=2, action_group=nameStr)
                rotXCurve = action.fcurves.new(rotAnimPath, index=0, action_group=nameStr)
                rotYCurve = action.fcurves.new(rotAnimPath, index=1, action_group=nameStr)
                rotZCurve = action.fcurves.new(rotAnimPath, index=2, action_group=nameStr)
                sclXCurve = action.fcurves.new(sclAnimPath, index=0, action_group=nameStr)
                sclYCurve = action.fcurves.new(sclAnimPath, index=1, action_group=nameStr)
                sclZCurve = action.fcurves.new(sclAnimPath, index=2, action_group=nameStr)

                if len(level['ANIMOBJECT']) > 1:
                    self.scene.frame_start = level['ANIMOBJECT'][1][0]

                for key in level['ANIMOBJECT']:
                    
                    if len(key) < 10:
                        meshObject.pie_object_prop.animTime = key[0]
                        meshObject.pie_object_prop.animCycle = key[1]
                        continue

                    locXCurve.keyframe_points.insert(key[0], key[1] * 0.00001)
                    locYCurve.keyframe_points.insert(key[0], key[2] * 0.00001)
                    locZCurve.keyframe_points.insert(key[0], key[3] * 0.00001)
                    rotXCurve.keyframe_points.insert(key[0], key[4] * 0.0000174533)
                    rotYCurve.keyframe_points.insert(key[0], key[5] * 0.0000174533)
                    rotZCurve.keyframe_points.insert(key[0], key[6] * 0.0000174533)
                    sclXCurve.keyframe_points.insert(key[0], key[7])
                    sclYCurve.keyframe_points.insert(key[0], key[8])
                    sclZCurve.keyframe_points.insert(key[0], key[9])

                    if self.scene.frame_end < key[0]:
                        self.scene.frame_end = key[0]


            armatureObject.select_set(True)
            bpy.context.view_layer.objects.active = armatureObject

            poseBone.rotation_mode = 'XYZ'

            bpy.ops.object.mode_set(mode='OBJECT', toggle=False)

            bpy.data.armatures[pieFile].bones[boneName].use_relative_parent = True

            self.pie_loadProperties(pieParse, armatureObject, pieFile)

    
    def pie_loadProperties(self, pieParse, pieObject, pieName):

        objProp = pieObject.pie_object_prop

        objProp.pieName = pieName.rsplit('.pie', 1)[0]

        def getTypeFlags(num):
            flags = []
            while num != 0:
                flags.append(num % 10)
                num = num // 10
            return flags

        objProp.texture = pieParse['TEXTURE']
        objProp.normal = pieParse['NORMALMAP']
        objProp.specular = pieParse['SPECULARMAP']

        for event in pieParse['EVENT']:
            if event[0] is '1':
                objProp.event1 = event[1]
            elif event[0] is '2':
                objProp.event2 = event[1]
            elif event[0] is '3':
                objProp.event3 = event[1]

        flags = getTypeFlags(pieParse['TYPE'])

        if flags[0] is 1:
            objProp.adrOff = True
            objProp.adrOn = False
            objProp.pmr = False
        elif flags[0] is 2:
            objProp.adrOff = False
            objProp.adrOn = True
            objProp.pmr = False
        elif flags[0] is 3:
            objProp.adrOff = True
            objProp.adrOn = True
            objProp.pmr = False
        elif flags[0] is 4:
            objProp.adrOff = False
            objProp.adrOn = False
            objProp.pmr = True
        elif flags[0] is 5:
            objProp.adrOff = True
            objProp.adrOn = False
            objProp.pmr = True
        elif flags[0] is 6:
            objProp.adrOff = False
            objProp.adrOn = True
            objProp.pmr = True
        elif flags[0] is 7:
            objProp.adrOff = True
            objProp.adrOn = True
            objProp.pmr = True
        else:
            objProp.adrOff = False
            objProp.adrOn = False
            objProp.pmr = False
        
        if len(flags) > 1:
            if flags[1] is 1:
                objProp.roll = True
                objProp.pitch = False
            elif flags[1] is 2:
                objProp.roll = False
                objProp.pitch = True
            elif flags[1] is 3:
                objProp.roll = True
                objProp.pitch = True
            else:
                objProp.roll = False
                objProp.pitch = False
        
        if len(flags) > 2:
            if flags[2] is 2:
                objProp.reserved = True
            else:
                objProp.reserved = False
        
        if len(flags) > 3:
            if flags[3] is 1:
                objProp.stretch = True
            else:
                objProp.stretch = False
        
        if len(flags) > 4:
            if flags[4] is 1:
                objProp.tcMask = True
            else:
                objProp.tcMask = False

    def pie_import(self, scene, pieDir, pieMesh):
        self.scene = scene

        pieParse = self.pie_parse(pieDir + '\\\\' + pieMesh)

        self.pie_generateBlenderObjects(pieParse)
