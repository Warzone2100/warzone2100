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
from .shared import *
from bpy.types import Scene, Object
from bpy.props import \
    BoolProperty as BoolProp, \
    PointerProperty as PointerProp, \
    IntProperty as IntProp, \
    FloatProperty as FloatProp, \
    EnumProperty as EnumProp, \
    StringProperty as StrProp, \
    CollectionProperty as ColProp


bl_info = {
    'name': 'pie_addon for .pie format: used by Warzone 2100',
    'author': 'John Wharton',
    'version': (1, 2, 0),
    'blender': (2, 90, 0),
    'description': 'Allows Blender to import/export models in the .pie format',
    'category': 'Import-Export',
}

DESC_TEXPROP_NAME = f'''The given file name for the texture'''
DESC_TEXPROP_SLOT = f'''Determines which slot the texture fills in the shader'''
DESC_TEXPROP_TILESET = f'''Specifies which tileset this texture should be used for'''

DESC_ROOT_EVENT1 = f'''Active animation event'''
DESC_ROOT_EVENT2 = f'''Firing animation event'''
DESC_ROOT_EVENT3 = f'''Death animation event'''
DESC_ROOT_INTERPOLATE = f'''Toggles sub-keyframe interpolation for the anim object(s) of the Root or Level'''

DESC_LEV_ANIMTIME = f'''Amount of time each animation cycles takes to complete'''
DESC_LEV_ANIMCYCLE = f'''Number of times an animation will repeat.\n"0" indicates that the animation will loop indefinitely'''
DESC_LEV_EXPNORNMAL = f'''Indicates if the mesh's vertex normals are exported'''

DESC_IMPORT_FILEPATH = f'''File path used for importing a .pie file'''
DESC_EXPORT_DIRECTORY = f'''File directory used for exporting .pie files'''


def texMapSlotPropGet(self):
    try:
        return self['slot']
    except KeyError:
        self['slot'] = 0
        return self['slot']


def texMapSlotPropSet(self, val):
    for txm in self.id_data.pie_tex_maps:
        if self.as_pointer() == txm.as_pointer():
            continue

        if e_TexMapSlot[val][0] == txm.slot and self.tileset == txm.tileset:
            return

    self['slot'] = val


def texMapTilesetPropGet(self):
    try:
        return self['tileset']
    except KeyError:
        self['tileset'] = 0
        return self['tileset']


def texMapTilesetPropSet(self, val):
    for txm in self.id_data.pie_tex_maps:
        if self.as_pointer() == txm.as_pointer():
            continue

        if self.slot == txm.slot and val == int(txm.tileset):
            return

    self['tileset'] = val


class PIE_TexMapProps(bpy.types.PropertyGroup):
    name: StrProp(
        options=set(), name='Name', description=DESC_TEXPROP_NAME
    )
    slot: EnumProp(
        options=set(), name='Slot', description=DESC_TEXPROP_SLOT,
        get=texMapSlotPropGet, set=texMapSlotPropSet,
        items=e_TexMapSlot,
    )
    tileset: EnumProp(
        options=set(), name='Tileset', description=DESC_TEXPROP_TILESET,
        get=texMapTilesetPropGet, set=texMapTilesetPropSet,
        items=e_TexMapTileset,
    )


class PIE_TexMapList(bpy.types.UIList):
    bl_idname = 'UI_UL_PIE_TexMap'

    def draw_item(self, context, layout, data, item, icon, active_data, active_propname, index, flt_flag):
        if self.layout_type in {'DEFAULT', 'COMPACT'}:
            row = layout.row()
            split = row.split()
            split.ui_units_x = 3.5
            split.prop(item, 'slot', text='', emboss=False)
            split = row.split()
            split.ui_units_x = 4.0
            split.prop(item, 'tileset', text='', emboss=False)
            row.prop(item, 'name', text='', emboss=False, icon_value=icon)


class PIE_TexMapPanel(bpy.types.Panel):
    bl_idname = 'OBJECT_PT_PIE_TexMap'
    bl_label = 'PIE Texture Maps'
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = 'object'
    bl_mode = 'edit'
    bl_parent_id = 'OBJECT_PT_PIE_ObjectPanel'

    @classmethod
    def poll(cls, context):
        ob = context.object
        root_ob = findPieRootObject(ob)
        return ob and (ob == root_ob or (
            root_ob is not None and
            root_ob.pie_object_prop.pieVersion == '4' and
            ob.pie_object_prop.pieType == 'LEVEL' and
            ob.type == "MESH"
        ))

    def draw(self, context):
        layout = self.layout
        ob = context.object
        length = len(ob.pie_tex_maps)

        root_ob = findPieRootObject(ob)
        if int(root_ob.pie_object_prop.pieVersion) < 4:
            layout.label(icon="INFO", text="NOTE: This is for defining texture maps only in PIE 4")

        rows = 2
        if length > 1:
            rows = 4

        row = layout.row()
        col = row.column()
        col.template_list(
            'UI_UL_PIE_TexMap', 'pie_tex_maps', ob, 'pie_tex_maps',
            ob, 'pie_tex_maps_index', rows=rows
        )
        col = row.column(align=True)
        col.operator('pie.tex_map_add', icon='ADD', text='')

        if length > 0:
            col.operator('pie.tex_map_remove', icon='REMOVE', text='')
        if length > 1:
            s = 'pie.tex_map_move'
            col.separator()
            col.operator(s, icon='TRIA_UP', text='').shift = -1
            col.operator(s, icon='TRIA_DOWN', text='').shift = 1


class PIE_TexMapOperationAdd(bpy.types.Operator):
    bl_idname = 'pie.tex_map_add'
    bl_label = 'Add texture animation group'
    bl_description = 'Adds a texture animation group for the .pie model'

    def invoke(self, context, event):
        ob = context.object
        unusedSlot = getPieUnusedTexMapSlot(ob)

        if unusedSlot is not None:
            texMap = ob.pie_tex_maps.add()
            texMap.name = 'texture_name.png'
            texMap['slot'], texMap['tileset'] = unusedSlot

        ob.pie_tex_maps_index = len(ob.pie_tex_maps) - 1

        return {'FINISHED'}


class PIE_TexMapOperationRemove(bpy.types.Operator):
    bl_idname = 'pie.tex_map_remove'
    bl_label = 'Remove texture animation group'
    bl_description = 'Removes the active texture animation group'

    def invoke(self, context, event):
        ob = context.object
        mapIndex = ob.pie_tex_maps_index

        ob.pie_tex_maps.remove(mapIndex)

        if ob.pie_tex_maps_index == len(ob.pie_tex_maps):
            ob.pie_tex_maps_index = ob.pie_tex_maps_index - 1

        return {'FINISHED'}


class PIE_TexMapOperationMove(bpy.types.Operator):
    bl_idname = 'pie.tex_map_move'
    bl_label = 'Move texture animation group'
    bl_description = 'Moves the active texture animation group'

    shift: IntProp(name='shift', default=0)

    def invoke(self, context, event):
        ob = context.object
        ii = ob.pie_tex_maps_index
        iiShift = ii + self.shift

        if iiShift < len(ob.pie_tex_maps) and iiShift >= 0:
            ob.pie_tex_maps.move(ii, iiShift)
            ob.pie_tex_maps_index += self.shift

        return {'FINISHED'}


class PIE_TexAnimGrpOptions(bpy.types.PropertyGroup):
    name: StrProp(
        name='name', default='Texture Animation Group'
    )
    imageCount: IntProp(
        options=set(), min=0
    )
    imageRate: IntProp(
        options=set(), min=0, default=1
    )
    imageWidth: FloatProp(
        options=set(), min=0.0, max=1.0, default=1.0, subtype='FACTOR'
    )
    imageHeight: FloatProp(
        options=set(), min=0.0, max=1.0, default=1.0, subtype='FACTOR'
    )


class PIE_TexAnimGrpPanel(bpy.types.Panel):
    bl_idname = 'OBJECT_PT_PIE_TexAnimGrp'
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
                ob, 'pie_tex_anim_grps_index', rows=rows
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

            texAnimGrpIndex = ob.pie_tex_anim_grps_index

            if texAnimGrpIndex >= 0 and texAnimGrpIndex < length:
                texAnimGrp = ob.pie_tex_anim_grps[texAnimGrpIndex]

                layout.use_property_split = True

                row = layout.row()
                row.operator('pie.tex_anim_grp_set', text='Set')
                row.operator('pie.tex_anim_grp_select', text='Select')
                layout.separator()

                layout.prop(texAnimGrp, 'imageCount', text='Images')
                layout.prop(texAnimGrp, 'imageRate', text='Rate')
                main = layout.row(align=True)
                main.use_property_split = False
                split = main.split(factor=0.4, align=True)
                row = split.row(align=True)
                row.alignment = 'RIGHT'
                row.label(text='Width/Height')
                row = split.row(align=True)
                row.prop(texAnimGrp, 'imageWidth', text='')
                row.prop(texAnimGrp, 'imageHeight', text='')
                main.separator(factor=3.5)


class PIE_TexAnimGrpOperationAdd(bpy.types.Operator):
    bl_idname = 'pie.tex_anim_grp_add'
    bl_label = 'Add texture animation group'
    bl_description = 'Adds a texture animation group for the .pie model'

    def invoke(self, context, event):
        ob = context.object
        name = 'Texture Animation Group ' + str(len(ob.pie_tex_anim_grps))

        texAnimGrp = ob.pie_tex_anim_grps.add()
        texAnimGrp.name = name

        ob.pie_tex_anim_grps_index = len(ob.pie_tex_anim_grps) - 1

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
        tagLayer = getTexAnimGrp(bm)
        grpIndex = ob.pie_tex_anim_grps_index

        for face in bm.faces:
            val = face[tagLayer]
            if val == grpIndex:
                face[tagLayer] = -1
            elif val > grpIndex:
                face[tagLayer] = val - 1

        ob.pie_tex_anim_grps.remove(grpIndex)

        if ob.pie_tex_anim_grps_index == len(ob.pie_tex_anim_grps):
            ob.pie_tex_anim_grps_index = ob.pie_tex_anim_grps_index - 1

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
        grpIndex = ob.pie_tex_anim_grps_index
        grpIndexShift = grpIndex + self.shift

        if grpIndexShift < len(ob.pie_tex_anim_grps) and grpIndexShift >= 0:
            obMode = ob.mode == 'OBJECT'
            bpy.ops.object.mode_set(mode='EDIT', toggle=False)
            bm = bmesh.from_edit_mesh(ob.data)
            tagLayer = getTexAnimGrp(bm)

            ob.pie_tex_anim_grps.move(grpIndex, grpIndexShift)

            for face in bm.faces:
                val = face[tagLayer]
                if val == grpIndex:
                    face[tagLayer] = grpIndexShift
                elif val == grpIndexShift:
                    face[tagLayer] = grpIndex

            ob.pie_tex_anim_grps_index += self.shift

            if obMode:
                bpy.ops.object.mode_set(mode='OBJECT', toggle=False)

        return {'FINISHED'}


class PIE_TexAnimGrpOperationSelect(bpy.types.Operator):
    bl_idname = 'pie.tex_anim_grp_select'
    bl_label = 'Select texture animation group'
    bl_description = 'Selects faces assigned to the texture animation group'

    @classmethod
    def poll(cls, context):
        return context.object and context.object.mode == 'EDIT'

    def invoke(self, context, event):
        ob = context.object
        bm = bmesh.from_edit_mesh(ob.data)
        tagLayer = getTexAnimGrp(bm)

        for face in bm.faces:
            face.select = True if face[tagLayer] == ob.pie_tex_anim_grps_index else False

        bmesh.update_edit_mesh(ob.data)

        return {'FINISHED'}


class PIE_TexAnimGrpOperationSet(bpy.types.Operator):
    bl_idname = 'pie.tex_anim_grp_set'
    bl_label = 'Set texture animation group'
    bl_description = 'Sets the texture animation group to the selected faces'

    @classmethod
    def poll(cls, context):
        return context.object and context.object.mode == 'EDIT'


    def invoke(self, context, event):
        ob = context.object
        bm = bmesh.from_edit_mesh(ob.data)
        tagLayer = getTexAnimGrp(bm)

        for face in bm.faces:
            if face.select:
                face[tagLayer] = ob.pie_tex_anim_grps_index

        return {'FINISHED'}


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
            layout.prop(field, 'event1', text='Event 1')
            layout.prop(field, 'event2', text='Event 2')
            layout.prop(field, 'event3', text='Event 3')
            layout.separator()
            self.ui_draw_flag_props(context, layout.box())
            layout.prop(field, "animInterpolate")
            if field.pieVersion in ['2', '3']:
                layout.prop(field, 'texture', text='Texture map')
                layout.prop(field, 'normal', text='Normal map')
                layout.prop(field, 'specular', text='Specular map')

        elif field.pieType == 'LEVEL':
            layout.prop(field, 'shadowType', text='Shadow Type')

            pieRootOb = findPieRootObject(context.object)
            if pieRootOb and pieRootOb.pie_object_prop.pieVersion == '4':
                layout.prop(field, 'overrideFlags', text='Level Override Flags')

                if field.overrideFlags:
                    self.ui_draw_flag_props(context, layout.box())

                layout.prop(field, 'overrideInterpolate')

                if field.overrideInterpolate:
                    layout.prop(field, "animInterpolate")

            layout.separator()
            layout.use_property_split = True
            # layout.prop(field, 'exportNormal', text='Export vertex normals')
            layout.prop(field, 'animTime', text='Animation Time')
            layout.prop(field, 'animCycle', text='Animation Cycles')

    def ui_draw_flag_props(self, context, layout):
        field = context.object.pie_object_prop
        col = layout.column_flow(columns=2)

        col.prop(field, 'adrOff', text='Additive rendering off')
        col.prop(field, 'adrOn', text='Additive rendering on')
        col.prop(field, 'pmr', text='Premultiplied rendering')
        col.prop(field, 'roll', text='Roll to face camera')
        col.prop(field, 'pitch', text='Pitch to face camera')
        col.prop(field, 'reserved', text='Reserved (Compatibility)')
        col.prop(field, 'stretch', text='Stretch to terrain')
        col.prop(field, 'tcMask', text='Team Colored')


class PIE_ObjectOptions(bpy.types.PropertyGroup):
    pieType: EnumProp(options=set(), items=e_ObjectPieType, default='NONE')

    pieVersion: EnumProp(options=set(), items=e_PieVersion, default='4')
    shadowType: EnumProp(
        options=set(), items=e_ObjectPieShadowType, default='DEFAULT'
    )
    event1: StrProp(options=set(), description=DESC_ROOT_EVENT1)
    event2: StrProp(options=set(), description=DESC_ROOT_EVENT2)
    event3: StrProp(options=set(), description=DESC_ROOT_EVENT3)

    overrideFlags: BoolProp(options=set(), default=False)
    adrOff: BoolProp(options=set())
    adrOn: BoolProp(options=set())
    pmr: BoolProp(options=set())
    roll: BoolProp(options=set())
    pitch: BoolProp(options=set())
    reserved: BoolProp(options=set(), default=True)
    stretch: BoolProp(options=set())
    tcMask: BoolProp(options=set())
    texture: StrProp(options=set())
    normal: StrProp(options=set())
    specular: StrProp(options=set())

    # exportNormal: BoolProp(
    #     options=set(), name='exportNormal', description=DESC_LEV_EXPNORNMAL
    # )
    overrideInterpolate: BoolProp(
        options=set(), name="Level Override Interpolate", default=False
    )
    animInterpolate: BoolProp(
        options=set(), name='Interpolate Animation', description=DESC_ROOT_INTERPOLATE, default=True
    )
    animTime: IntProp(
        options=set(), name='animTime', description=DESC_LEV_ANIMTIME
    )
    animCycle: IntProp(
        options=set(), name='animCycle', description=DESC_LEV_ANIMCYCLE
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
        description=DESC_EXPORT_DIRECTORY,
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
        description=DESC_IMPORT_FILEPATH,
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
    PIE_TexMapProps,
    PIE_TexMapList,
    PIE_TexMapOperationAdd,
    PIE_TexMapOperationRemove,
    PIE_TexMapOperationMove,
    PIE_TexMapPanel,
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
    Object.pie_tex_maps = ColProp(type=PIE_TexMapProps)
    Object.pie_tex_maps_index = IntProp(default=-1)
    Object.pie_tex_anim_grps = ColProp(type=PIE_TexAnimGrpOptions)
    Object.pie_tex_anim_grps_index = IntProp(default=-1)
    bpy.types.TOPBAR_MT_file_export.append(menu_pie_export)
    bpy.types.TOPBAR_MT_file_import.append(menu_pie_import)


def unregister():
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)
    bpy.types.TOPBAR_MT_file_export.remove(menu_pie_export)
    bpy.types.TOPBAR_MT_file_import.remove(menu_pie_import)


if __name__ == '__main__':
    register()
