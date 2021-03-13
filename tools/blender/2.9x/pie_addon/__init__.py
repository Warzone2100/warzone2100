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

from . import pie_export
from . import pie_import
import bpy
import bmesh


bl_info = {
    'name': 'pie_addon for .pie format: used by Warzone 2100',
    'author': 'John Wharton',
    'version': (1, 0, 0),
    'blender': (2, 90, 0),
    'description': 'Allows Blender to import and export models in the .pie format',
    'category': 'Import-Export',
}


class PIE_TextureAnimationGroupOptions(bpy.types.PropertyGroup):
    name : bpy.props.StringProperty(name='name', default='Texture Animation Group', options=set())

    texAnimImages: bpy.props.IntProperty(options=set(), min=0)
    texAnimRate:   bpy.props.IntProperty(options=set(), min=0)
    texAnimWidth:  bpy.props.IntProperty(options=set(), min=1, max=256)
    texAnimHeight: bpy.props.IntProperty(options=set(), min=0, max=256)
    texAnimFaces:  bpy.props.StringProperty(options=set())


class PIE_TextureAnimationGroupPanel(bpy.types.Panel):
    bl_idname = 'OBJECT_PT_PIE_TextureAnimationGroupPanel'
    bl_label = 'PIE Texture Animation Groups'
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = 'object'
    bl_mode = 'edit'
    bl_parent_id = 'OBJECT_PT_PIE_ObjectPanel'

    @classmethod
    def poll(cls, context):
        ob = context.object
        return ob and (ob.pie_object_prop == 'LEVEL') and (ob.type == "MESH")

    def draw(self, context):
        layout = self.layout
        ob = context.object

        if context.object.type == 'MESH':
            row = layout.row()
            col = row.column()
            col.template_list('UI_UL_list', 'pie_texture_animation_groups', ob, 'pie_texture_animation_groups', ob, 'pie_texture_animation_group_index', rows=3)
            col = row.column()
            col.operator('pie.texture_animation_group_add', icon='ADD', text='')
            col.operator('pie.texture_animation_group_remove', icon='REMOVE', text='')

            texAnimGroupIndex = ob.pie_texture_animation_group_index

            if texAnimGroupIndex >= 0 and texAnimGroupIndex < len(ob.pie_texture_animation_groups):
                texAnimGroup = ob.pie_texture_animation_groups[texAnimGroupIndex]

                layout.prop(texAnimGroup, 'name', text='Name')
                layout.use_property_split = True

                if ob.mode == 'EDIT':
                    split = layout.split()
                    row = split.row()
                    col = row.column()
                    col.operator('pie.texture_animation_group_set', text='Set')
                    row = split.row()
                    col = row.column()
                    col.operator('pie.texture_animation_group_select', text='Select')
                    layout.separator()

                layout.prop(texAnimGroup, 'texAnimImages', text='Texture Animation Images')
                layout.prop(texAnimGroup, 'texAnimRate',   text='Texture Animation Rate')
                layout.separator()
                layout.prop(texAnimGroup, 'texAnimWidth',  text='Texture Animation Width')
                layout.prop(texAnimGroup, 'texAnimHeight', text='Texture Animation Height')


class PIE_TextureAnimationGroupOperationAdd(bpy.types.Operator):
    bl_idname      = 'pie.texture_animation_group_add'
    bl_label       = 'Add texture animation group'
    bl_description = 'Adds a texture animation group for the .pie model'

    def invoke(self, context, event):
        ob = context.object

        texAnimGroup = ob.pie_texture_animation_groups.add()
        texAnimGroup.name = 'Texture Animation Group ' + str(len(ob.pie_texture_animation_groups))
        texAnimGroup.texAnimImages = 0
        texAnimGroup.texAnimRate = 1
        texAnimGroup.texAnimWidth = 256
        texAnimGroup.texAnimHeight = 256

        ob.pie_texture_animation_group_index = len(ob.pie_texture_animation_groups) - 1

        return {'FINISHED'}


class PIE_TextureAnimationGroupOperationRemove(bpy.types.Operator):
    bl_idname      = 'pie.texture_animation_group_remove'
    bl_label       = 'Remove texture animation group'
    bl_description = 'Removes the active texture animation group'

    def invoke(self, context, event):
        ob = context.object

        if ob.pie_texture_animation_group_index >= 0:
            ob.pie_texture_animation_groups.remove(ob.pie_texture_animation_group_index)

            if ob.pie_texture_animation_group_index is not 0:
                ob.pie_texture_animation_group_index -= 1

        return {'FINISHED'}


class PIE_TextureAnimationGroupOperationSelect(bpy.types.Operator):
    bl_idname      = 'pie.texture_animation_group_select'
    bl_label       = 'Select texture animation group'
    bl_description = 'Selects the faces assigned to the active texture animation group'

    def invoke(self, context, event):
        ob = context.object

        if ob.pie_texture_animation_groups[ob.pie_texture_animation_group_index].texAnimFaces is '':
            return {'FINISHED'}

        bm = bmesh.from_edit_mesh(ob.data)
        faces = bm.faces
        faceSetStr = ob.pie_texture_animation_groups[ob.pie_texture_animation_group_index].texAnimFaces
        faceSetInt = []

        for face in faces:
            face.select = False

        for face in faceSetStr.split():
            faceSetInt.append(int(face))

        ii = 0
        while ii < len(faceSetInt):
            bm.faces[int(faceSetInt[ii])].select = True
            ii += 1

        bmesh.update_edit_mesh(ob.data)

        return {'FINISHED'}


class PIE_TextureAnimationGroupOperationSet(bpy.types.Operator):
    bl_idname      = 'pie.texture_animation_group_set'
    bl_label       = 'Set texture animation group'
    bl_description = 'Sets the active texture animation group to the currently selected faces'

    def invoke(self, context, event):
        ob = context.object
        bm = bmesh.from_edit_mesh(ob.data)

        faces = bm.faces
        faceSet = ''

        ii = 0
        while ii < len(faces):
            if faces[ii].select is True:
                faceSet += ' {str} '.format(str=str(ii))

                jj = 0
                while jj < len(ob.pie_texture_animation_groups):
                    if jj is not ob.pie_texture_animation_group_index:
                        newFaceSet = ob.pie_texture_animation_groups[jj].texAnimFaces.replace(' {str} '.format(str=str(ii)), '')
                        ob.pie_texture_animation_groups[jj].texAnimFaces = newFaceSet
                    jj += 1

            ii += 1

        ob.pie_texture_animation_groups[ob.pie_texture_animation_group_index].texAnimFaces = faceSet

        return {'FINISHED'}


pie_object_enum = [
    ('NONE', 'None', 'None'),
    ('ROOT', 'Root', 'PIE root object'),
    ('LEVEL', 'Level', 'PIE level. These must always be a mesh and parented to a "Root" PIE object'),
    ('CONNECTOR', 'Connector', 'PIE connector. These must always be parented to a "Level" PIE object')
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
            box = layout.box()
            split = box.split()
            row = split.row()
            col = row.column()
            col.prop(field, 'adrOff', text='Additive rendering off')
            col.prop(field, 'adrOn', text='Additive rendering on')
            col.prop(field, 'pmr', text='Premultiplied rendering')
            col.prop(field, 'roll', text='Roll to face camera')
            row = split.row()
            col = row.column()
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
            layout.use_property_split = True
            #layout.prop(field, 'exportNormal', text='Export vertex normals')
            layout.prop(field, 'animTime', text='Animation Time')
            layout.prop(field, 'animCycle', text='Animation Cycles')


class PIE_ObjectOptions(bpy.types.PropertyGroup):

    pieType: bpy.props.EnumProperty(items=pie_object_enum, options=set(), default='NONE')

    adrOff: bpy.props.BoolProperty(name='adrOff', options=set())
    adrOn: bpy.props.BoolProperty(name='adrOn', options=set())
    pmr: bpy.props.BoolProperty(name='pmr', options=set())
    roll: bpy.props.BoolProperty(name='roll', options=set())
    pitch: bpy.props.BoolProperty(name='pitch', options=set())
    reserved: bpy.props.BoolProperty(name='reserved', options=set())
    stretch: bpy.props.BoolProperty(name='stretch', options=set())
    tcMask: bpy.props.BoolProperty(name='tcMask', options=set())
    texture: bpy.props.StringProperty(name='texture', options=set())
    normal: bpy.props.StringProperty(name='normal', options=set())
    specular: bpy.props.StringProperty(name='specular', options=set())
    event1: bpy.props.StringProperty(name='event1', options=set(), description='Active animation event')
    event2: bpy.props.StringProperty(name='event2', options=set(), description='Firing animation event')
    event3: bpy.props.StringProperty(name='event3', options=set(), description='Death animation event')

    #exportNormal: bpy.props.BoolProperty(name='exportNormal', options=set(), description='Indicates if the vertex normals of this mesh should be exported')
    animTime: bpy.props.IntProperty(name='animTime', options=set(), description='Indicates the amount of time each animation cycle should take to complete')
    animCycle: bpy.props.IntProperty(name='animCycle', options=set(), description='Indicates the number of times an animation should repeat. "0" indicates that the animation should loop indefinitely')


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
    rootDir: bpy.props.StringProperty(name='rootDir', default='C:\\Users\\John Wharton\\Documents\\', options=set())


class PIE_ExportOperationQuick(bpy.types.Operator):
    bl_idname = 'pie.export_quick'
    bl_label = 'Export .pie'
    bl_options = {'UNDO'}

    def invoke(self, context, event):
        scene = context.scene
        
        if scene.pie_export_prop.rootDir[len(scene.pie_export_prop.rootDir) - 1] is not '\\':
            scene.pie_export_prop.rootDir += '\\' 

        obs = []

        for ob in bpy.context.selected_objects:
            while ob.parent is not None and ob.pie_object_prop.pieType != "ROOT":
                ob = ob.parent
            if ob.pie_object_prop.pieType == "ROOT":
                if ob not in obs:
                    obs.append(ob)
                    ob.select_set(False)

        if len(obs) is 0:
            self.report({'ERROR'}, 'You must select at least 1 "Root" PIE object or an object which is parented to one before exporting')
        else:
            exporter.pie_export(scene, scene.pie_export_prop.rootDir, obs)

        return {'FINISHED'}


class PIE_ExportOperation(bpy.types.Operator):
    """Export a .pie file"""
    bl_idname = 'pie.export'
    bl_label = 'Export .pie to directory'
    bl_options = {'UNDO'}

    filename_ext = '.pie'
    filter_glob: bpy.props.StringProperty(default='*.pie', options={'HIDDEN'})

    directory: bpy.props.StringProperty(
        name = 'File Directory',
        description = 'File directory used for exporting .pie files',
        maxlen = 1023,
        default = ''
    )

    def execute(self, context):
        scene = context.scene

        obs = []

        for ob in bpy.context.selected_objects:
            while ob.parent is not None and ob.pie_object_prop.pieType != "ROOT":
                ob = ob.parent
            if ob.pie_object_prop.pieType == "ROOT":
                if ob not in obs:
                    obs.append(ob)
                    ob.select_set(False)

        if len(obs) is 0:
            self.report({'ERROR'}, 'You must select at least 1 "Root" PIE object or an object which is parented to one before exporting')
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
    pieFile: bpy.props.StringProperty(name='pieFile', default='', options=set())
    rootDir: bpy.props.StringProperty(name='rootDir', default='C:\\', options=set())


class PIE_ImportOperationQuick(bpy.types.Operator):
    bl_idname = 'pie.import_quick'
    bl_label = 'Import .pie'
    bl_options = {'UNDO'}

    def invoke(self, context, event):
        scene = context.scene

        for ob in bpy.data.objects:
            if scene.pie_import_prop.pieFile == ob.name:
                self.report({'ERROR'}, 'There is already an object which matches the filename "{file}"'.format(file=scene.pie_import_prop.pieFile))
                return {'FINISHED'}
            #else:
            #    for action in bpy.data.actions:
            #        if scene.pie_import_prop.pieFile + ' Anim' in action.name:
            #            self.report({'ERROR'}, 'There is already an animation which matches filename "{file}"'.format(file=scene.pie_import_prop.pieFile))

        importer.pie_import(scene, scene.pie_import_prop.rootDir, scene.pie_import_prop.pieFile)

        return {'FINISHED'}


class PIE_ImportOperation(bpy.types.Operator):
    """Import a .pie file"""
    bl_idname = 'pie.import'
    bl_label = 'Import .pie'
    bl_options = {'UNDO'}

    filename_ext = '.pie'
    filter_glob: bpy.props.StringProperty(default='*.pie', options={'HIDDEN'})

    filepath: bpy.props.StringProperty(
        name = 'File Path',
        description = 'File path used for importing a .pie file',
        maxlen = 1023,
        default = ''
    )

    def execute(self, context):
        scene = context.scene

        for ob in bpy.data.objects:
            if scene.pie_import_prop.pieFile == ob.name:
                self.report({'ERROR'}, 'There is already an object which matches filename "{file}"'.format(file=scene.pie_import_prop.pieFile))
                return {'FINISHED'}
            #else:
            #    for action in bpy.data.actions:
            #        if scene.pie_import_prop.pieFile + ' Anim' in action.name:
            #            self.report({'ERROR'}, 'There is already an animation which matches filename "{file}"'.format(file=scene.pie_import_prop.pieFile))

        importer.pie_import(scene, self.properties.filepath, '')

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
    PIE_TextureAnimationGroupOptions,
    PIE_TextureAnimationGroupOperationAdd,
    PIE_TextureAnimationGroupOperationRemove,
    PIE_TextureAnimationGroupOperationSelect,
    PIE_TextureAnimationGroupOperationSet,
    PIE_TextureAnimationGroupPanel,
)

def menu_pie_import(self, context):
    self.layout.operator(PIE_ImportOperation.bl_idname, text='Warzone 2100 Model (.pie)')

def menu_pie_export(self, context):
    self.layout.operator(PIE_ExportOperation.bl_idname, text='Warzone 2100 Model (.pie)')

def register():
    for cls in classes: bpy.utils.register_class(cls)
    bpy.types.Scene.pie_export_prop                    = bpy.props.PointerProperty(type=PIE_ExportOptions)
    bpy.types.Scene.pie_import_prop                    = bpy.props.PointerProperty(type=PIE_ImportOptions)
    bpy.types.Object.pie_object_prop                   = bpy.props.PointerProperty(type=PIE_ObjectOptions)
    bpy.types.Object.pie_texture_animation_groups      = bpy.props.CollectionProperty(type=PIE_TextureAnimationGroupOptions)
    bpy.types.Object.pie_texture_animation_group_index = bpy.props.IntProperty(options=set())
    bpy.types.TOPBAR_MT_file_export.append(menu_pie_export)
    bpy.types.TOPBAR_MT_file_import.append(menu_pie_import)

def unregister():
    for cls in reversed(classes): bpy.utils.unregister_class(cls)
    bpy.types.TOPBAR_MT_file_export.remove(menu_pie_export)
    bpy.types.TOPBAR_MT_file_import.remove(menu_pie_import)

if __name__ == '__main__':
    register()