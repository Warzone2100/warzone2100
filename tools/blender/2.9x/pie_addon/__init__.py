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
from . import pie_export
from . import pie_import
from .shared import getTexAnimGrp
from bpy.types import Scene, Object
from bpy.props import \
    BoolProperty as BoolProp, \
    PointerProperty as PointerProp, \
    IntProperty as IntProp, \
    EnumProperty as EnumProp, \
    StringProperty as StrProp, \
    CollectionProperty as ColProp


bl_info = {
    'name': 'pie_addon for .pie format: used by Warzone 2100',
    'author': 'John Wharton',
    'version': (1, 1, 0),
    'blender': (2, 90, 0),
    'description': 'Allows Blender to import/export models in the .pie format',
    'category': 'Import-Export',
}


class PIE_TexAnimGrpOptions(bpy.types.PropertyGroup):
    name: StrProp(name='name', default='Texture Animation Group')

    texAnimImages: IntProp(options=set(), min=0)
    texAnimRate: IntProp(options=set(), min=0, default=1)
    texAnimWidth: IntProp(options=set(), min=1, max=256, default=256)
    texAnimHeight: IntProp(options=set(), min=0, max=256, default=256)


class PIE_TexAnimGrpPanel(bpy.types.Panel):
    bl_idname = 'OBJECT_PT_PIE_TexAnimGrpPanel'
    bl_label = 'PIE Texture Animation Groups'
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = 'object'
    bl_mode = 'edit'
    bl_parent_id = 'OBJECT_PT_PIE_ObjectPanel'

    @classmethod
    def poll(cls, context):
        ob = context.object
        return ob and (
            ob.pie_object_prop.pieType == 'LEVEL' and 
            ob.type == "MESH"
        )

    def draw(self, context):
        layout = self.layout
        ob = context.object

        if context.object.type == 'MESH':

            length = len(ob.pie_tex_anim_grps)

            rows = 2
            if length > 1:
                rows = 4

            row = layout.row()
            col = row.column()
            col.template_list(
                'UI_UL_list', 'pie_tex_anim_grps', ob, 'pie_tex_anim_grps',
                ob, 'pie_tex_anim_grp_index', rows=rows
            )
            col = row.column(align=True)
            col.operator('pie.tex_anim_grp_add', icon='ADD', text='')

            if length > 0:
                col.operator('pie.tex_anim_grp_remove', icon='REMOVE', text='')
            if length > 1:
                s = 'pie.tex_anim_grp_move'
                col.separator()
                col.operator(s, icon='TRIA_UP', text='').shift = -1
                col.operator(s, icon='TRIA_DOWN', text='').shift = 1

            texAnimGrpIndex = ob.pie_tex_anim_grp_index

            if texAnimGrpIndex >= 0 and texAnimGrpIndex < length:
                texAnimGrp = ob.pie_tex_anim_grps[texAnimGrpIndex]

                layout.prop(texAnimGrp, 'name', text='Name')
                layout.use_property_split = True

                if ob.mode == 'EDIT':
                    split = layout.split()
                    row = split.row()
                    col = row.column()
                    col.operator('pie.tex_anim_grp_set', text='Set')
                    row = split.row()
                    col = row.column()
                    col.operator('pie.tex_anim_grp_select', text='Select')
                    layout.separator()

                s = 'Texture Animation '
                layout.prop(texAnimGrp, 'texAnimImages', text=s + 'Images')
                layout.prop(texAnimGrp, 'texAnimRate', text=s + 'Rate')
                layout.separator()
                layout.prop(texAnimGrp, 'texAnimWidth', text=s + 'Width')
                layout.prop(texAnimGrp, 'texAnimHeight', text=s + 'Height')


class PIE_TexAnimGrpOperationAdd(bpy.types.Operator):
    bl_idname = 'pie.tex_anim_grp_add'
    bl_label = 'Add texture animation group'
    bl_description = 'Adds a texture animation group for the .pie model'

    def invoke(self, context, event):
        ob = context.object
        name = 'Texture Animation Group ' + str(len(ob.pie_tex_anim_grps))

        texAnimGrp = ob.pie_tex_anim_grps.add()
        texAnimGrp.name = name

        return {'FINISHED'}


class PIE_TexAnimGrpOperationRemove(bpy.types.Operator):
    bl_idname = 'pie.tex_anim_grp_remove'
    bl_label = 'Remove texture animation group'
    bl_description = 'Removes the active texture animation group'

    def invoke(self, context, event):
        ob = context.object
        obMode = ob.mode == 'OBJECT'
        bpy.ops.object.mode_set(mode='EDIT', toggle=False)
        bm = bmesh.from_edit_mesh(ob.data)
        faces = bm.faces

        grpIndex = ob.pie_tex_anim_grp_index

        for face in faces:
            for ii in range(len(ob.pie_tex_anim_grps)):
                if ii == grpIndex:
                    layer = getTexAnimGrp(bm, str(ii))
                    face[layer] = 0
                elif ii > grpIndex:
                    layer = getTexAnimGrp(bm, str(ii))
                    layerVal = face[layer]
                    layerShift = getTexAnimGrp(bm, str(ii - 1))
                    face[layerShift] = layerVal

        ob.pie_tex_anim_grps.remove(grpIndex)

        if ob.pie_tex_anim_grp_index != 0:
            ob.pie_tex_anim_grp_index -= 1

        if obMode:
            bpy.ops.object.mode_set(mode='OBJECT', toggle=False)

        return {'FINISHED'}


class PIE_TexAnimGrpOperationMove(bpy.types.Operator):
    bl_idname = 'pie.tex_anim_grp_move'
    bl_label = 'Move texture animation group'
    bl_description = 'Moves the active texture animation group'

    shift: IntProp(name='shift', default=0)

    def invoke(self, context, event):
        ob = context.object
        ii = ob.pie_tex_anim_grp_index
        iiShift = ii + self.shift

        if iiShift < len(ob.pie_tex_anim_grps) and iiShift >= 0:
            obMode = ob.mode == 'OBJECT'
            bpy.ops.object.mode_set(mode='EDIT', toggle=False)
            bm = bmesh.from_edit_mesh(ob.data)
            faces = bm.faces

            ob.pie_tex_anim_grps.move(ii, iiShift)

            layer = getTexAnimGrp(bm, str(ii))
            layerShift = getTexAnimGrp(bm, str(iiShift))
            for face in faces:
                temp = face[layer]
                tempShift = face[layerShift]
                face[layer] = tempShift
                face[layerShift] = temp

            ob.pie_tex_anim_grp_index += self.shift

            if obMode:
                bpy.ops.object.mode_set(mode='OBJECT', toggle=False)

        return {'FINISHED'}


class PIE_TexAnimGrpOperationSelect(bpy.types.Operator):
    bl_idname = 'pie.tex_anim_grp_select'
    bl_label = 'Select texture animation group'
    bl_description = 'Selects faces assigned to the texture animation group'

    def invoke(self, context, event):
        ob = context.object
        bm = bmesh.from_edit_mesh(ob.data)
        layer = getTexAnimGrp(bm, str(ob.pie_tex_anim_grp_index))
        faces = bm.faces

        for face in faces:
            face.select = True if face[layer] == 1 else False

        bmesh.update_edit_mesh(ob.data)

        return {'FINISHED'}


class PIE_TexAnimGrpOperationSet(bpy.types.Operator):
    bl_idname = 'pie.tex_anim_grp_set'
    bl_label = 'Set texture animation group'
    bl_description = 'Sets the texture animation group to the selected faces'

    def invoke(self, context, event):
        ob = context.object
        bm = bmesh.from_edit_mesh(ob.data)
        faces = bm.faces

        for ii in range(len(ob.pie_tex_anim_grps)):
            layer = getTexAnimGrp(bm, str(ii))
            for face in faces:
                if ii == ob.pie_tex_anim_grp_index:
                    face[layer] = 1 if face.select else 0
                elif face.select:
                    face[layer] = 0

        return {'FINISHED'}


pie_ob_enum = [
    ('NONE', 'None', 'None'),
    ('ROOT', 'Root', 'PIE root object'),
    ('LEVEL', 'Level', (
        'PIE level. These must always be a mesh '
        'and parented to a "Root" PIE object'
        )),
    ('SHADOW', 'Shadow (WZ2100 4.0+)', (
        'PIE level shadow. These must always be a '
        'mesh and parented to a "Level" PIE object'
        )),
    ('CONNECTOR', 'Connector', (
        'PIE level connector. These must always '
        'be parented to a "Level" PIE object'
        )),
]

pie_shadow_enum = [
    ('DEFAULT', 'Default', 'The shadow will default to the level mesh'),
    ('CUSTOM', 'Custom', (
        'Define the level shadow with a PIE "Shadow" mesh object. '
        'This object must be parented to the level object'
        )),
    # ('CONVEXHULL', 'Convex Hull', (
    #     'Define the level shadow with a convex hull of the level mesh'
    #     )),
]


class PIE_ObjectPanel(bpy.types.Panel):
    bl_idname = 'OBJECT_PT_PIE_ObjectPanel'
    bl_label = 'PIE Object'
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = 'object'

    def draw(self, context):
        layout = self.layout

        field = context.object.pie_object_prop

        layout.prop(field, 'pieType', text='PIE Object Type')

        if field.pieType == 'ROOT':
            layout.prop(field, 'pieVersion', text='PIE Version')
            box = layout.box()
            col = box.column_flow(columns=2)
            col.prop(field, 'adrOff', text='Additive rendering off')
            col.prop(field, 'adrOn', text='Additive rendering on')
            col.prop(field, 'pmr', text='Premultiplied rendering')
            col.prop(field, 'roll', text='Roll to face camera')
            col.prop(field, 'pitch', text='Pitch to face camera')
            col.prop(field, 'reserved', text='Reserved (Compatibility)')
            col.prop(field, 'stretch', text='Stretch to terrain')
            col.prop(field, 'tcMask', text='Team Colored')
            layout.prop(field, 'texture', text='Texture map')
            layout.prop(field, 'normal', text='Normal map')
            layout.prop(field, 'specular', text='Specular map')
            layout.prop(field, 'event1', text='Event 1')
            layout.prop(field, 'event2', text='Event 2')
            layout.prop(field, 'event3', text='Event 3')
        elif field.pieType == 'LEVEL':
            layout.prop(field, 'shadowType', text='Shadow Type')
            layout.use_property_split = True
            # layout.prop(field, 'exportNormal', text='Export vertex normals')
            layout.prop(field, 'animTime', text='Animation Time')
            layout.prop(field, 'animCycle', text='Animation Cycles')


pie_version_enum = [
    ('2', 'PIE 2', 'Not generally recommended.'),
    ('3', 'PIE 3', 'The latest PIE version.'),
]


class PIE_ObjectOptions(bpy.types.PropertyGroup):

    pieType: EnumProp(options=set(), items=pie_ob_enum, default='NONE')
    pieVersion: EnumProp(options=set(), items=pie_version_enum, default='3')
    shadowType: EnumProp(
        options=set(), items=pie_shadow_enum, default='DEFAULT'
    )

    adrOff: BoolProp(options=set())
    adrOn: BoolProp(options=set())
    pmr: BoolProp(options=set())
    roll: BoolProp(options=set())
    pitch: BoolProp(options=set())
    reserved: BoolProp(options=set())
    stretch: BoolProp(options=set())
    tcMask: BoolProp(options=set())
    texture: StrProp(options=set())
    normal: StrProp(options=set())
    specular: StrProp(options=set())
    event1: StrProp(options=set(), description='Active animation event')
    event2: StrProp(options=set(), description='Firing animation event')
    event3: StrProp(options=set(), description='Death animation event')

    # exportNormal: BoolProp(
    #     name='exportNormal', 
    #     description='Indicates if the mesh's vertex normals are exported'
    # )
    animTime: IntProp(
        name='animTime', 
        description='Amount of time each animation cycle takes to complete'
    )
    animCycle: IntProp(
        name='animCycle', 
        description=(
            'Number of times an animation will repeat. '
            '"0" indicates that the animation will loop indefinitely'
        )
    )


exporter = pie_export.Exporter()


class PIE_ExportPanel(bpy.types.Panel):
    bl_idname = 'OBJECT_PT_PIE_ExportPanel'
    bl_label = 'PIE Export'
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = 'scene'

    def draw(self, context):
        layout = self.layout
        scene = context.scene

        layout.prop(scene.pie_export_prop, 'rootDir', text='Directory')
        layout.operator('pie.export_quick', text='Export selected .pie models')


class PIE_ExportOptions(bpy.types.PropertyGroup):
    rootDir: StrProp(name='Root Directory', default='')


class PIE_ExportOperationQuick(bpy.types.Operator):
    bl_idname = 'pie.export_quick'
    bl_label = 'Export .pie'
    bl_options = {'UNDO'}

    def invoke(self, context, event):
        scene = context.scene
        rootDir = scene.pie_export_prop.rootDir
        
        if rootDir[len(rootDir) - 1] != '\\':
            scene.pie_export_prop.rootDir += '\\' 

        obs = []

        for ob in bpy.context.selected_objects:
            pieType = ob.pie_object_prop.pieType
            while ob.parent is not None and pieType != "ROOT":
                ob = ob.parent
            if ob.pie_object_prop.pieType == "ROOT":
                if ob not in obs:
                    obs.append(ob)
                    ob.select_set(False)

        if len(obs) == 0:
            self.report(
                {'ERROR'}, (
                    'You must select at least 1 "Root" PIE object or an '
                    'object which is parented to one before exporting'
                )
            )
        else:
            exporter.pie_export(scene, rootDir, obs)

        return {'FINISHED'}


class PIE_ExportOperation(bpy.types.Operator):
    """Export a .pie file"""
    bl_idname = 'pie.export'
    bl_label = 'Export .pie to directory'
    bl_options = {'UNDO'}

    filename_ext = '.pie'
    filter_glob: StrProp(default='*.pie', options={'HIDDEN'})
    directory: StrProp(
        name='File Directory',
        description='File directory used for exporting .pie files',
        maxlen=1023,
        default=''
    )

    def execute(self, context):
        scene = context.scene

        obs = []

        for ob in bpy.context.selected_objects:
            pieType = ob.pie_object_prop.pieType
            while ob.parent is not None and pieType != "ROOT":
                ob = ob.parent
            if ob.pie_object_prop.pieType == "ROOT":
                if ob not in obs:
                    obs.append(ob)
                    ob.select_set(False)

        if len(obs) == 0:
            self.report(
                {'ERROR'}, (
                    'You must select at least 1 "Root" PIE object or an '
                    'object which is parented to one before exporting'
                )
            )
        else:
            exporter.pie_export(scene, self.properties.directory, obs)

        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)

        return {'RUNNING_MODAL'}


importer = pie_import.Importer()


class PIE_ImportPanel(bpy.types.Panel):
    bl_idname = 'OBJECT_PT_PIE_ImportPanel'
    bl_label = 'PIE Import'
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = 'scene'

    def draw(self, context):
        layout = self.layout
        scene = context.scene

        layout.prop(scene.pie_import_prop, 'pieFile', text='.pie File')
        layout.prop(scene.pie_import_prop, 'rootDir', text='Directory')
        layout.operator('pie.import_quick', text='Import .pie Model')


class PIE_ImportOptions(bpy.types.PropertyGroup):
    pieFile: StrProp(name='pieFile', default='')
    rootDir: StrProp(name='rootDir', default='')


class PIE_ImportOperationQuick(bpy.types.Operator):
    bl_idname = 'pie.import_quick'
    bl_label = 'Import .pie'
    bl_options = {'UNDO'}

    def invoke(self, context, event):
        scene = context.scene
        fileName = scene.pie_import_prop.pieFile
        rootDir = scene.pie_import_prop.rootDir

        for ob in bpy.data.objects:
            if fileName != ob.name:
                continue
            self.report(
                {'ERROR'}, (
                    'There is already an object which matches the filename '
                    + fileName
                )
            )
            return {'FINISHED'}

        importer.pie_import_quick(scene, rootDir, fileName)

        return {'FINISHED'}


class PIE_ImportOperation(bpy.types.Operator):
    """Import a .pie file"""
    bl_idname = 'pie.import'
    bl_label = 'Import .pie'
    bl_options = {'UNDO'}

    filename_ext = '.pie'
    filter_glob: StrProp(default='*.pie', options={'HIDDEN'})

    filepath: StrProp(
        name='File Path',
        description='File path used for importing a .pie file',
        maxlen=1023,
        default=''
    )

    def execute(self, context):

        importer.pie_import(context.scene, self.properties.filepath)

        return {'FINISHED'}

    def invoke(self, context, event):
        
        context.window_manager.fileselect_add(self)

        return {'RUNNING_MODAL'}


classes = (
    PIE_ExportOperation,
    PIE_ExportOperationQuick,
    PIE_ExportOptions,
    PIE_ExportPanel,
    PIE_ImportOperation,
    PIE_ImportOperationQuick,
    PIE_ImportOptions,
    PIE_ImportPanel,
    PIE_ObjectOptions,
    PIE_ObjectPanel,
    PIE_TexAnimGrpOptions,
    PIE_TexAnimGrpOperationAdd,
    PIE_TexAnimGrpOperationRemove,
    PIE_TexAnimGrpOperationMove,
    PIE_TexAnimGrpOperationSelect,
    PIE_TexAnimGrpOperationSet,
    PIE_TexAnimGrpPanel,
)


def menu_pie_import(self, context):
    self.layout.operator('pie.import', text='Warzone 2100 Model (.pie)')


def menu_pie_export(self, context):
    self.layout.operator('pie.export', text='Warzone 2100 Model (.pie)')


def register():
    for cls in classes: 
        bpy.utils.register_class(cls)
    Scene.pie_export_prop = PointerProp(type=PIE_ExportOptions)
    Scene.pie_import_prop = PointerProp(type=PIE_ImportOptions)
    Object.pie_object_prop = PointerProp(type=PIE_ObjectOptions)
    Object.pie_tex_anim_grps = ColProp(type=PIE_TexAnimGrpOptions)
    Object.pie_tex_anim_grp_index = IntProp()
    bpy.types.TOPBAR_MT_file_export.append(menu_pie_export)
    bpy.types.TOPBAR_MT_file_import.append(menu_pie_import)


def unregister():
    for cls in reversed(classes): 
        bpy.utils.unregister_class(cls)
    bpy.types.TOPBAR_MT_file_export.remove(menu_pie_export)
    bpy.types.TOPBAR_MT_file_import.remove(menu_pie_import)


if __name__ == '__main__':
    register()