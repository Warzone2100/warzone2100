# pie-addon - Blender Import-Export for .pie file format

**Blender** addon to **import and export** models in the **`.pie`** format, which is used in the game **Warzone 2100**

Created by *John Wharton*

## Installation

1. Download the most recent version at **(https://github.com/Warzone2100/warzone2100/tree/master/tools/blender/2.9x)**
2. Install the addon use one of these methods:
* __Option A__ (automatic):
    * From the Blender menu, navigate to `Edit -> Preferences -> Add-ons`.
    * Click on the `Install` button and select the zipped folder containing the addon.
* __Option B__ (manual):
    * Extract the zipped files to `pie_addon` directory.
    * Place the newly made folder into the following location (Adjust for the installed version of Blender):
        * Windows:\
            `%APPDATA%\Blender Foundation\Blender\2.9x\scripts\addons`
        * Linux:\
            `~/.config/blender/2.9x/scripts/addons`
        * *(If the addon does not appear as a choice in Blender's preferences click the `Refresh` button or restart the application.)*
3. Activate the addon in Blender's preferences by toggling the checkbox for the `pie_addon` entry in the Add-ons tab.

## Features

The scripts in this addon currently support importing and exporting the following information:

* PIE Type
* PIE Texture
* PIE Normal Map
* PIE Specular Map
* PIE Events
* PIE Levels
    * PIE Points
    * PIE Polygons*
        * PIE Textured Polygons
        * PIE Animated Polygons
    * PIE Connectors
    * PIE Anim Objects*
    * PIE Shadow Points
    * PIE Shadow Polygons

*1: N-gons should not be used in exported meshes due to UV corruption. Quads will be triangulated correctly and are acceptable for export from Blender to PIE.
*2: There may be some cases Blender will not interpret rotation keys in the same manner as the game.

## Usage

This addon adds the following panels to the Scene tab of the properties editor:

1. PIE Export
    * Used for exporting models to the .pie format.
    * Before exporting, you must select the root object (typically an armature) of the model which you desire to export.
    * The output file will be located in the system path specified in the `Directory` property, with the name of the file being the name of the root object.
    * You can export multiple objects at the same time by selecting all of them before executing the export operation.
2. PIE Import
    * Used for importing .pie models into Blender.
    * Specify the system path in the `Directory` property and the file name in the `.pie File` property. Be sure to include the file extension `.pie`.

The following panel can be found in the Object tab of the properties editor:

* PIE Object
    * Objects can be assigned one of these `PIE Object Type` values.
        1. `None`:
            * Objects with this type will be ignored when exporting, but they may still be used in the Blender scene to manipulate objects which are eligable for PIE exporting.
        2. `Root`:
            * This type is used to define the generic values of a PIE model, such as its version, rendering flags, textures, and events.
        3. `Level`:
            * This type is used to define the mesh and animation properties which are specific to each level such as animation rate/cycles and texture animation data for particular sets of faces. These should always be mesh objects, and also should always be within the heirarchy of a `Root` PIE object.
        4. `Shadow`:
            * This type is used to define the shadow of a level. These should always be mesh objects, and should also be parented directly to a `Level` PIE object. The level's `Shadow Type` must be set to `Custom` in order to export PIE shadows.
        5. `Connector`:
            * This type is used to define the location of a connector on a level. These should be parented directly to a `Level` PIE object.