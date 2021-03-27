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


class Exporter():

    def pie_export(self, scene, path, obs):
        self.scene = scene

        for ob in obs:

            nameStr = ''

            if '.pie' in ob.name:
                nameStr = ob.name
            else:
                nameStr = ob.name + '.pie'
            
            pieFile = open(path + nameStr, 'w')

            print('Exporting {pie} to {path}'.format(pie=nameStr, path=path))

            ob.select_set(True)

            obProp = ob.pie_object_prop

            pieFile.write('PIE 3')

            objType = 0

            if obProp.adrOff:
                objType +=     1
            if obProp.adrOn:
                objType +=     2
            if obProp.pmr:
                objType +=     4
            if obProp.roll:
                objType +=    10
            if obProp.pitch:
                objType +=    20
            if obProp.reserved:
                objType +=   200
            if obProp.stretch:
                objType +=  1000
            if obProp.tcMask:
                objType += 10000

            pieFile.write('\nTYPE {num}'.format(num=objType))

            if obProp.texture != '':
                pieFile.write('\nTEXTURE 0 {str} 0 0'.format(str=obProp.texture))

            if obProp.normal != '':
                pieFile.write('\nNORMALMAP 0 {str}'.format(str=obProp.normal))

            if obProp.specular != '':
                pieFile.write('\nSPECULARMAP 0 {str}'.format(str=obProp.specular))

            if obProp.event1 != '':
                pieFile.write('\nEVENT 1 {str}'.format(str=obProp.event1))

            if obProp.event2 != '':
                pieFile.write('\nEVENT 2 {str}'.format(str=obProp.event2))

            if obProp.event3 != '':
                pieFile.write('\nEVENT 3 {str}'.format(str=obProp.event3))

            if ob.children:

                level = 0

                levelObjects = []
                
                for child in ob.children:
                    for child2 in child.children:
                        for child3 in child2.children:
                            if child3.pie_object_prop.pieType == 'LEVEL' and child3.type == "MESH":
                                levelObjects.append(child3)
                        if child2.pie_object_prop.pieType == 'LEVEL' and child2.type == "MESH":
                            levelObjects.append(child2)
                    if child.pie_object_prop.pieType == 'LEVEL' and child.type == "MESH":
                        levelObjects.append(child)

                if len(levelObjects) > 0:
                    print('Exporting {pie} levels'.format(pie=nameStr))
                    pieFile.write('\nLEVELS {num}'.format(num=len(levelObjects)))


                for child in levelObjects:
                    childProp = child.pie_object_prop

                    depsgraph = bpy.context.evaluated_depsgraph_get()

                    meshOb = child.evaluated_get(depsgraph)
                    mesh = meshOb.data

                    bm = bmesh.new()
                    bm.from_mesh(mesh)
                    bmesh.ops.triangulate(bm, faces=bm.faces)

                    level += 1

                    print('Exporting {pie} level {num}'.format(pie=nameStr, num=level))
                    pieFile.write('\nLEVEL {num}'.format(num=level))

                    if bm.verts:
                        print('Exporting {pie} level {num} points'.format(pie=nameStr, num=level))
                        pieFile.write('\nPOINTS {num}'.format(num=len(bm.verts)))

                        for vertice in bm.verts:
                            val = []

                            for num in vertice.co:
                                num = round(num / 0.01, 4)

                                if abs(num - round(num)) <= 0.000105:
                                    num = int(round(num))

                                val.append(num)

                            pieFile.write('\n\t{x} {z} {y}'.format(x=val[0], y=val[1], z=val[2]))

                    if bm.faces:
                        faceStr = ''
                        #normalStr = '
                        #print('Exporting {pie} level {num} normals = {prop}'.format(pie=nameStr, num=level, prop=child.pie_object_prop.exportNormal))
                        print('Exporting {pie} level {num} polygons'.format(pie=nameStr, num=level))

                        faceStr += '\nPOLYGONS {num}'.format(num=len(bm.faces))

                        #if child.pie_object_prop.exportNormal is True:
                        #    normalStr += '\nNORMALS {num}'.format(num=len(child.data.polygons))
                        
                        ii = 0
                        for face in bm.faces:

                            #if child.pie_object_prop.exportNormal is True:
                            #    verticeNormals = []
                            #    for vertex in poly.vertices:
                            #        verticeNormals.append(child.data.vertices[vertex].normal)
                            #        #print('vertex: {normal}'.format(normal=child.data.vertices[vertex].normal))
                            #    #print('face: {normal}'.format(normal=poly.normal))
                            #    normalStr += '\n\t{x1} {y2} {z1} {x2} {y2} {z2} {x3} {y3} {z3}'.format(
                            #        x1=round(verticeNormals[0][0], 4), y1=round(verticeNormals[0][1], 4), z1=round(verticeNormals[0][2], 4),
                            #        x2=round(verticeNormals[1][0], 4), y2=round(verticeNormals[1][1], 4), z2=round(verticeNormals[1][2], 4),
                            #        x3=round(verticeNormals[2][0], 4), y3=round(verticeNormals[2][1], 4), z3=round(verticeNormals[2][2], 4),
                            #    )

                            uv_layer = bm.loops.layers.uv.verify()
                            uvOut = [[], [], []]
                            
                            jj = 0
                            for loop in face.loops:
                                uvOut[jj].append(round(loop[uv_layer].uv[0], 6))
                                uvOut[jj].append(round(loop[uv_layer].uv[1], 6))
                                jj += 1

                            success = False
                            for texAnimGroup in child.pie_texture_animation_groups:
                                if str(ii) in texAnimGroup.texAnimFaces:
                                    success = True
                                    faceStr += '\n\t{type} 3 {v1} {v2} {v3} {tagImg} {tagRate} {tagW} {tagH} {uvx1} {uvy1} {uvx2} {uvy2} {uvx3} {uvy3}'.format(
                                        type=4200,
                                        v1=face.verts[0].index, v2=face.verts[1].index, v3=face.verts[2].index,
                                        tagImg=texAnimGroup.texAnimImages, tagRate=texAnimGroup.texAnimRate,
                                        tagW=texAnimGroup.texAnimWidth, tagH=texAnimGroup.texAnimHeight,
                                        uvx1=round(uvOut[0][0], 4), uvy1=-round(uvOut[0][1], 4) + 1,
                                        uvx2=round(uvOut[1][0], 4), uvy2=-round(uvOut[1][1], 4) + 1,
                                        uvx3=round(uvOut[2][0], 4), uvy3=-round(uvOut[2][1], 4) + 1,
                                    )
                                    break
                            if success is False:
                                faceStr += '\n\t{type} 3 {v1} {v2} {v3} {uvx1} {uvy1} {uvx2} {uvy2} {uvx3} {uvy3}'.format(
                                    type=200,
                                    v1=face.verts[0].index, v2=face.verts[1].index, v3=face.verts[2].index,
                                    uvx1=round(uvOut[0][0], 4), uvy1=-round(uvOut[0][1], 4) + 1,
                                    uvx2=round(uvOut[1][0], 4), uvy2=-round(uvOut[1][1], 4) + 1,
                                    uvx3=round(uvOut[2][0], 4), uvy3=-round(uvOut[2][1], 4) + 1,
                                )

                            ii += 1

                        #if child.pie_object_prop.exportNormal is True:
                        #    pieFile.write(normalStr)

                        pieFile.write(faceStr)

                    connectorObjects = []
                    for connector in child.children:
                        if connector.pie_object_prop.pieType == 'CONNECTOR':
                            connectorObjects.append(connector)

                    if connectorObjects:
                        print('Exporting {pie} level {num} connectors'.format(pie=nameStr, num=level))
                        pieFile.write('\nCONNECTORS {num}'.format(num=len(connectorObjects)))
                        for connector in connectorObjects:

                            val = []

                            for num in [connector.location.x * 100, connector.location.y * 100, connector.location.z * 100]:
                                num = round(num, 4)

                                if abs(num - round(num)) <= 0.0001:
                                    num = int(round(num))

                                val.append(num)

                            pieFile.write('\n\t{x} {y} {z}'.format(x=val[0], y=val[1], z=val[2]))

                    print(ob.animation_data)

                    if ob.animation_data:

                        endFrame = 0

                        success = False
                        for fcurve in ob.animation_data.action.fcurves:

                            if '["{name}"]'.format(name=child.name) in fcurve.data_path or '["{name}"]'.format(name=child.parent) in fcurve.data_path or '["{name}"]'.format(name=child.parent_bone) in fcurve.data_path:
                                success = True
                                if fcurve.keyframe_points[-1].co[0] > endFrame:
                                    endFrame = int(round(fcurve.keyframe_points[-1].co[0]))

                        if success == True:

                            restFrame = bpy.context.scene.frame_current

                            print('Exporting {pie} level {num} animation = True'.format(pie=nameStr, num=level))
                            pieFile.write('\nANIMOBJECT {time} {cycles} {frames}'.format(time=childProp.animTime, cycles=childProp.animCycle, frames=endFrame + 1))

                            ii = 0
                            while ii <= endFrame:
                                bpy.context.scene.frame_set(ii)

                                childMatrix = child.matrix_world.decompose()
                                exportMatrix = child.matrix_world - ob.matrix_world

                                mLoc = exportMatrix.decompose()[0]
                                mRot = childMatrix[1]
                                mRot = mRot.to_euler('YZX')
                                mScl = childMatrix[2]
                                loc, rot, scl = [[], [], []]

                                for val in [mLoc[0], mLoc[1], mLoc[2]]:
                                    loc.append(round(val * 100000))

                                for val in [mRot[0], mRot[1], mRot[2]]:
                                    rot.append(round(val * 57295.755))

                                for val in [mScl[0], mScl[1], mScl[2]]:
                                    if abs(val - round(val)) < 0.000149:
                                        scl.append(round(val, 1))
                                    else:
                                        scl.append(round(val, 4))

                                sFrame = ''.ljust(abs(len(str(    ii)) - 3) + 8, ' ')
                                sLocX  = ''.ljust(abs(len(str(loc[0])) - 8) + 4, ' ')
                                sLocY  = ''.ljust(abs(len(str(loc[1])) - 8),     ' ')
                                sLocZ  = ''.ljust(abs(len(str(loc[2])) - 8),     ' ')
                                sRotX  = ''.ljust(abs(len(str(rot[0])) - 8),     ' ')
                                sRotY  = ''.ljust(abs(len(str(rot[1])) - 8),     ' ')
                                sRotZ  = ''.ljust(abs(len(str(rot[2])) - 8),     ' ')
                                sSclX  = ''.ljust(abs(len(str(scl[0])) - 8),     ' ')
                                sSclY  = ''.ljust(abs(len(str(scl[1])) - 8),     ' ')
                                sSclZ  = ''.ljust(abs(len(str(scl[2])) - 8),     ' ')

                                pieFile.write('\n{sFrame}{frame}{sLocX}{locX}{sLocY}{locY}{sLocZ}{locZ}{sRotX}{rotX}{sRotY}{rotY}{sRotZ}{rotZ}{sSclX}{sclX}{sSclY}{sclY}{sSclZ}{sclZ}'
                                .format(
                                    sFrame=sFrame, frame=ii,
                                    sLocX=sLocX, sLocY=sLocY, sLocZ=sLocZ,
                                    locX=loc[0], locY=loc[1], locZ=loc[2],
                                    sRotX=sRotX, sRotY=sRotY, sRotZ=sRotZ,
                                    rotX=rot[0], rotY=rot[1], rotZ=rot[2],
                                    sSclX=sSclX, sSclY=sSclY, sSclZ=sSclZ,
                                    sclX=scl[0], sclY=scl[1], sclZ=scl[2],
                                    )
                                )

                                ii += 1

                            bpy.context.scene.frame_set(restFrame)
                        else:
                            print('Exporting {pie} level {num} animation = False'.format(pie=nameStr, num=level))
                    else:
                            print('Exporting {pie} level {num} animation = False'.format(pie=nameStr, num=level))

                    exportShadow = False
                    if childProp.shadowType == 'CUSTOM':
                        for shadow in child.children:
                            if shadow.pie_object_prop.pieType == 'SHADOW':
                                exportShadow = True
                                shadowBm = bmesh.new()
                                shadowBm.from_mesh(shadow.evaluated_get(depsgraph).data)
                                bmesh.ops.triangulate(shadowBm, faces=shadowBm.faces)
                                break

                    #elif childProp.shadowType == 'CONVEXHULL':
                        #exportShadow = True
                        #shadowBm = bmesh.new()
                        #shadowBm.from_mesh(mesh)
                        #bmesh.ops.convex_hull(shadowBm, input=shadowBm.verts)

                        #copyData = bpy.data.meshes.new('test')

                        #shadowBm.to_mesh(copyData)

                        #copy = bpy.data.objects.new('test', copyData)
                        #bpy.context.collection.objects.link(copy)

                    if exportShadow is not False:

                        if shadowBm.verts:
                            print('Exporting {pie} level {num} shadow points'.format(pie=nameStr, num=level))
                            pieFile.write('\nSHADOWPOINTS {num}'.format(num=len(shadowBm.verts)))

                            for vertice in shadowBm.verts:
                                val = []

                                for num in vertice.co:
                                    num = round(num / 0.01, 4)

                                    if abs(num - round(num)) <= 0.000105:
                                        num = int(round(num))

                                    val.append(num)

                                pieFile.write('\n\t{x} {z} {y}'.format(x=val[0], y=val[1], z=val[2]))

                        if shadowBm.faces:
                            print('Exporting {pie} level {num} shadow polygons'.format(pie=nameStr, num=level))
                            pieFile.write('\nSHADOWPOLYGONS {num}'.format(num=len(shadowBm.faces)))

                            for face in shadowBm.faces:
                                pieFile.write('\n\t0 3 {v1} {v2} {v3}'.format(v1=face.verts[0].index, v2=face.verts[1].index, v3=face.verts[2].index,))

                        shadowBm.free()

                    bm.free()


            pieFile.close()