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
from .shared import getTexAnimGrp


class Exporter():

    def pie_export(self, scene, path, obs):
        self.scene = scene

        for ob in obs:

            nameStr = ob.name if '.pie' in ob.name else ob.name + '.pie'

            pieFile = open(path + nameStr, 'w')

            print('Exporting {pie} to {path}'.format(pie=nameStr, path=path))

            ob.select_set(True)

            obProp = ob.pie_object_prop

            version = obProp.pieVersion

            pieFile.write('PIE ' + version)

            objType = 0

            if obProp.adrOff:
                objType += 1
            if obProp.adrOn:
                objType += 2
            if obProp.pmr:
                objType += 4
            if obProp.roll:
                objType += 10
            if obProp.pitch:
                objType += 20
            if obProp.reserved:
                objType += 200
            if obProp.stretch:
                objType += 1000
            if obProp.tcMask:
                objType += 10000

            pieFile.write(
                '\nTYPE {num}'.format(num=objType)
            )

            if obProp.texture:
                pieFile.write(
                    '\nTEXTURE 0 {str} 0 0'.format(str=obProp.texture)
                )

            if obProp.normal:
                pieFile.write(
                    '\nNORMALMAP 0 {str}'.format(str=obProp.normal)
                )

            if obProp.specular:
                pieFile.write(
                    '\nSPECULARMAP 0 {str}'.format(str=obProp.specular)
                )

            if obProp.event1:
                pieFile.write(
                    '\nEVENT 1 {str}'.format(str=obProp.event1)
                )

            if obProp.event2:
                pieFile.write(
                    '\nEVENT 2 {str}'.format(str=obProp.event2)
                )

            if obProp.event3:
                pieFile.write(
                    '\nEVENT 3 {str}'.format(str=obProp.event3)
                )

            if ob.children:

                lvl = 0

                def getDescendantObs(parent):
                    arr = []
                    for child in parent.children:
                        if (child.pie_object_prop.pieType == 'LEVEL'
                                and child.type == 'MESH'):
                            arr.append(child)
                            childDescendants = getDescendantObs(child)
                            arr.extend(childDescendants)
                    return arr

                lvlObs = getDescendantObs(ob)

                if lvlObs:
                    print('Exporting {pie} levels'.format(pie=nameStr))
                    pieFile.write('\nLEVELS {num}'.format(num=len(lvlObs)))

                for child in lvlObs:
                    childProp = child.pie_object_prop

                    depsgraph = bpy.context.evaluated_depsgraph_get()
                    depOb = child.evaluated_get(depsgraph)
                    evalMe = bpy.data.meshes.new_from_object(depOb)
                    evalOb = bpy.data.objects.new(nameStr + ' eval', evalMe)
                    bpy.context.collection.objects.link(evalOb)

                    bpy.context.view_layer.objects.active = evalOb
                    bpy.ops.object.mode_set(mode='EDIT', toggle=False)
                    bm = bmesh.from_edit_mesh(evalMe)

                    lvl += 1

                    print(
                        'Exporting {pie} level {num}'
                        .format(pie=nameStr, num=lvl)
                    )
                    pieFile.write(
                        '\nLEVEL {num}'.format(num=lvl)
                    )

                    if bm.verts:
                        print(
                            'Exporting {pie} level {num} points'
                            .format(pie=nameStr, num=lvl)
                        )
                        pieFile.write(
                            '\nPOINTS {num}'.format(num=len(bm.verts))
                        )

                        for vertice in bm.verts:
                            val = []

                            for num in vertice.co:
                                num = round(num / 0.01, 4)

                                if abs(num - round(num)) <= 0.000105:
                                    num = int(round(num))

                                val.append(num)

                            pieFile.write(
                                '\n\t{x} {z} {y}'
                                .format(x=val[0], y=val[1], z=val[2])
                            )

                    if bm.faces:
                        faceStr = ''
                        print(
                            'Exporting {pie} level {num} polygons'
                            .format(pie=nameStr, num=lvl)
                        )

                        faceStr += '\nPOLYGONS {num}'.format(num=len(bm.faces))

                        def adjustToIntUv(num):
                            result = num * 256
                            while result < 0:
                                result += 256
                            while result > 256:
                                result -= 256
                            return round(result)
                        
                        for face in bm.faces:

                            uvLayer = bm.loops.layers.uv.verify()
                            uvOut = [[], [], []]
                            
                            for ii, loop in enumerate(face.loops):
                                uvOut[ii].append(round(loop[uvLayer].uv[0], 6))
                                uvOut[ii].append(round(loop[uvLayer].uv[1], 6))

                            uvx1 = round(uvOut[0][0], 4)
                            uvy1 = round(-uvOut[0][1] + 1, 4)
                            uvx2 = round(uvOut[1][0], 4)
                            uvy2 = round(-uvOut[1][1] + 1, 4)
                            uvx3 = round(uvOut[2][0], 4)
                            uvy3 = round(-uvOut[2][1] + 1, 4)

                            if version == '2':
                                uvx1 = adjustToIntUv(uvx1)
                                uvy1 = adjustToIntUv(uvy1)
                                uvx2 = adjustToIntUv(uvx2)
                                uvy2 = adjustToIntUv(uvy2)
                                uvx3 = adjustToIntUv(uvx3)
                                uvy3 = adjustToIntUv(uvy3)

                            success = False

                            for ii, tag in enumerate(child.pie_tex_anim_grps):
                                layer = getTexAnimGrp(bm, str(ii))
                                if face[layer] == 1:
                                    success = True
                                    faceStr += ('\n\t{type} 3 {v1} {v2} {v3} '
                                                '{tagImg} {tagRate} {tagW} '
                                                '{tagH} {uvx1} {uvy1} {uvx2} '
                                                '{uvy2} {uvx3} {uvy3}').format(
                                        type=4200,
                                        v1=face.verts[0].index,
                                        v2=face.verts[1].index,
                                        v3=face.verts[2].index,
                                        tagImg=tag.texAnimImages,
                                        tagRate=tag.texAnimRate,
                                        tagW=tag.texAnimWidth,
                                        tagH=tag.texAnimHeight,
                                        uvx1=uvx1,
                                        uvy1=uvy1,
                                        uvx2=uvx2,
                                        uvy2=uvy2,
                                        uvx3=uvx3,
                                        uvy3=uvy3,
                                    )
                                    break
                            if success is False:
                                faceStr += ('\n\t{type} 3 {v1} {v2} {v3} '
                                            '{uvx1} {uvy1} {uvx2} {uvy2} '
                                            '{uvx3} {uvy3}').format(
                                    type=200,
                                    v1=face.verts[0].index,
                                    v2=face.verts[1].index,
                                    v3=face.verts[2].index,
                                    uvx1=uvx1,
                                    uvy1=uvy1,
                                    uvx2=uvx2,
                                    uvy2=uvy2,
                                    uvx3=uvx3,
                                    uvy3=uvy3,
                                )

                        # if child.pie_object_prop.exportNormal is True:
                        #     pieFile.write(normalStr)

                        pieFile.write(faceStr)

                    connectorObs = []
                    for connector in child.children:
                        if connector.pie_object_prop.pieType == 'CONNECTOR':
                            connectorObs.append(connector)

                    if connectorObs:
                        print(
                            'Exporting {pie} level {num} connectors'
                            .format(pie=nameStr, num=lvl)
                        )
                        pieFile.write(
                            '\nCONNECTORS {num}'.format(num=len(connectorObs))
                        )
                        for connector in connectorObs:

                            val = []
                            loc = connector.location

                            for num in [loc.x * 100, loc.y * 100, loc.z * 100]:
                                num = round(num, 4)

                                if abs(num - round(num)) <= 0.0001:
                                    num = int(round(num))

                                val.append(num)

                            pieFile.write(
                                '\n\t{x} {y} {z}'.format(
                                    x=val[0], y=val[1], z=val[2]
                                )
                            )

                    if ob.animation_data:

                        endFrame = 0

                        success = False
                        for fcurve in ob.animation_data.action.fcurves:

                            cName = '["{name}"]'.format(name=child.name)
                            pName = '["{name}"]'.format(name=child.parent)
                            bName = '["{name}"]'.format(name=child.parent_bone)

                            if (cName in fcurve.data_path or
                                    pName in fcurve.data_path or
                                    bName in fcurve.data_path):
                                success = True
                                kp = fcurve.keyframe_points
                                if kp[-1].co[0] > endFrame:
                                    endFrame = int(round(kp[-1].co[0])) + 1

                        if success:

                            restFrame = bpy.context.scene.frame_current

                            print(
                                'Exporting {pie} level {num} animation = True'
                                .format(pie=nameStr, num=lvl)
                            )
                            pieFile.write(
                                '\nANIMOBJECT {time} {cycles} {frames}'.format(
                                    time=childProp.animTime,
                                    cycles=childProp.animCycle,
                                    frames=endFrame
                                )
                            )

                            for ii in range(endFrame):
                                bpy.context.scene.frame_set(ii)

                                childMatrix = child.matrix_world.decompose()
                                exMatrix = child.matrix_world - ob.matrix_world

                                mLoc = exMatrix.decompose()[0]
                                mRot = childMatrix[1]
                                mRot = mRot.to_euler('YZX')
                                mScl = childMatrix[2]
                                L, r, s = [[], [], []]

                                for val in mLoc:
                                    L.append(str(round(val * 100000)))

                                for val in mRot:
                                    r.append(str(round(val * 57295.755)))

                                for val in mScl:
                                    if abs(val - round(val)) < 0.000149:
                                        s.append(str(round(val, 1)))
                                    else:
                                        s.append(str(round(val, 4)))

                                def formSpaces(num, spaces):
                                    return abs(len(num) - spaces)

                                sFr = ''.ljust(formSpaces(str(ii), 3) + 8)
                                sLX = ''.ljust(formSpaces(L[0], 8) + 4)
                                sLY = ''.ljust(formSpaces(L[1], 8))
                                sLZ = ''.ljust(formSpaces(L[2], 8))
                                sRX = ''.ljust(formSpaces(r[0], 8))
                                sRY = ''.ljust(formSpaces(r[1], 8))
                                sRZ = ''.ljust(formSpaces(r[2], 8))
                                sSX = ''.ljust(formSpaces(s[0], 8))
                                sSY = ''.ljust(formSpaces(s[1], 8))
                                sSZ = ''.ljust(formSpaces(s[2], 8))

                                pieFile.write(
                                    '\n' + sFr + str(ii) +
                                    sLX + L[0] + sLY +
                                    L[1] + sLZ + L[2] +
                                    sRX + r[0] + sRY +
                                    r[1] + sRZ + r[2] +
                                    sSX + s[0] + sSY +
                                    s[1] + sSZ + s[2]
                                )

                            bpy.context.scene.frame_set(restFrame)
                        else:
                            print(
                                'Exporting {pie} level {num} animation = False'
                                .format(pie=nameStr, num=lvl)
                            )
                    else:
                        print(
                            'Exporting {pie} level {num} animation = False'
                            .format(pie=nameStr, num=lvl)
                        )

                    exportShadow = False
                    if childProp.shadowType == 'CUSTOM':
                        for sh in child.children:
                            if sh.pie_object_prop.pieType != 'SHADOW':
                                continue
                            exportShadow = True
                            shBm = bmesh.new()
                            shBm.from_mesh(sh.evaluated_get(depsgraph).data)
                            bmesh.ops.triangulate(shBm, faces=shBm.faces)
                            break

                    # elif childProp.shadowType == 'CONVEXHULL':
                    #     exportShadow = True
                    #     shadowBm = bmesh.new()
                    #     shadowBm.from_mesh(mesh)
                    #     bmesh.ops.convex_hull(shadowBm, input=shadowBm.verts)
                    #
                    #     copyData = bpy.data.meshes.new('test')
                    #
                    #     shadowBm.to_mesh(copyData)
                    #
                    #     copy = bpy.data.objects.new('test', copyData)
                    #     bpy.context.collection.objects.link(copy)

                    if exportShadow is not False:

                        if shBm.verts:
                            print(
                                'Exporting {pie} level {num} shadow points'
                                .format(pie=nameStr, num=lvl)
                            )
                            pieFile.write(
                                '\nSHADOWPOINTS {num}'
                                .format(num=len(shBm.verts))
                            )

                            for vertice in shBm.verts:
                                val = []

                                for num in vertice.co:
                                    num = round(num / 0.01, 4)

                                    if abs(num - round(num)) <= 0.000105:
                                        num = int(round(num))

                                    val.append(num)

                                pieFile.write('\n\t{x} {z} {y}'.format(
                                    x=val[0], y=val[1], z=val[2])
                                )

                        if shBm.faces:
                            print(
                                'Exporting {pie} level {num} shadow polygons'
                                .format(pie=nameStr, num=lvl)
                            )
                            pieFile.write('\nSHADOWPOLYGONS {num}'.format(
                                num=len(shBm.faces))
                            )

                            for face in shBm.faces:
                                pieFile.write('\n\t0 3 {v1} {v2} {v3}'.format(
                                    v1=face.verts[0].index,
                                    v2=face.verts[1].index,
                                    v3=face.verts[2].index,)
                                )

                        shBm.free()

                    bpy.ops.object.mode_set(mode='OBJECT', toggle=False)
                    bpy.data.objects.remove(evalOb)

                    pieFile.write('\n')

            pieFile.close()