PIE 4
=======

Description
-----------

The PIE format is a custom model format originally created by Pumpkin.

It has gone through three iterations since the original commercial release:
- PIE 2 was used until version 2.3, and is still supported
- PIE 3 was the standard from version 2.3 through 4.3, and is still supported
- PIE 4 was introduced in Warzone 2100 version 4.4, and is the recommended format for new models

PIE 2 uses integer coordinate values ranging from 0 to 256, corresponding to 1024 pixels for texture pages.
For example, the coordinates 128,256 refer to pixel coordinates 512,1024.

PIE 3 uses floating point UV coordinates which range from 0 to 1, usually with six digits.
Thus, the coordinates 0.111111,1.000000 represent offset 113.777777,256 for a picture 1024x1024 pixels large.
For texture repetition, UV coordinates can be negative numbers. Only Helicopter models currently contain those, with a precision of eight digits.

PIE 4 adds level overrides for global directives, the TCMASK directive, support for in-file comments (denoted by #), and removes some old (deprecated) functionality.

Format
------

## GLOBAL DIRECTIVES

### PIE

> PIE 4

The first line specifies the version number.

### TYPE

> TYPE 211

This indicates the type of the file through a hexadecimal combination of the flags 0x200, 0x10 and 0x1.
The following flags are available:

* `0x00001` -- Disables additive rendering
* `0x00002` -- Enables additive rendering
* `0x00004` -- Enables premultiplied rendering

* `0x00010` -- Rolls object to face the camera. Used for projectiles shaped like a cylinder.
* `0x00020` -- Pitches object to completely face the camera. Used for projectiles shaped like a sphere.

* `0x00200` -- Reserved for backward compatibility.
* `0x01000` -- Specifies that the model should not be stretched to fit terrain. For defensive buildings that have a deep foundation.
* `0x10000` -- Specifies the usage of the TCMask feature, for which a texture named 'page-N_tcmask.png' (*N* being a number) should be used together with the model's ordinary texture. This flag replaced old team coloration methods (read ticket #851).

### INTERPOLATE

> INTERPOLATE 0

Optional. Specifies if the model wants to have interpolated frames. Default is set to interpolate.

### TEXTURE

> TEXTURE 0 page-7-barbarians-arizona.png

This sets the texture page for the model. Each file must contain exactly one such line.
In theory you could leave out this line, but in practice that makes your models useless.
Note that the exact texture page file may be modified by WRF files during level loading, so that the texture matches the tileset.

The first parameter (the zero) specifies the tileset:
* `0` -- default, arizona
* `1` -- urban
* `2` -- rockies

In general, you should _always_ specify a TEXTURE for tileset 0, and optionally include additional TEXTURE directives if overrides for the urban or rockies tilesets are desired.

The second gives you the filename of the texture page, which
* must only contain the characters [a-zA-Z0-9.\_\\-] ([a-z] is not meant to include any lowercase character not found in the English alphabet).
* should start with "page-NN-" for correct handling of dynamic texture replacement.
* should end with the letters ".png".

### TCMASK

> TCMASK 0 page-7_tcmask.png

Optional. As above, but this sets the tcmask texture page for this model.

### NORMALMAP

> NORMALMAP 0 page-7-barbarians-arizona_normal.png

Optional. As above, but this sets the normal map texture page for the model.

### SPECULARMAP

> SPECULARMAP 0 page-7-barbarians-arizona.png

Optional. As above, but this sets the specular map texture page for the model.

### EVENT

> EVENT type filename.pie

An animation event associated with this model. If the event type is triggered, the model is
replaced with the specified model for the duration of the event. The following event types are defined:

* 1 -- Active event. What this means depends on the type of model. For droids this means movement,
    while for power generators it means they are linked to a power source.
* 2 -- Firing. The model is firing at some enemy.
* 3 -- Dying. The model is dying. You (almost) always want to make sure animation cycles for this model is set to 1 for the specified model -- if it is zero, it will never die!

### LEVELS

> LEVELS 1

This gives the number of meshes that are contained in this model. Each mesh can be animated separately in ANI files.

## LEVEL DIRECTIVES

### LEVEL

> LEVEL 1

This starts the model description for mesh 1. Repeat the below as necessary while incrementing the value above as needed.

### (OPTIONAL) PER-LEVEL OVERRIDES

The following global directives can be specified at the LEVEL scope to override the associated global directive for the level:
- TYPE
- INTERPOLATE
- TEXTURE
- TCMASK
- NORMALMAP
- SPECULARMAP

`TYPE` and `INTERPOLATE` should come first, and be specified in that order. The `TEXTURE`, `TCMASK`, `NORMALMAP`, and `SPECULARMAP` follow, but can be present in any order.
All are optional and, if omitted, the associated global directive will be used (if set).

### POINTS

> POINTS n

This starts a list of vertex coordinates (points) with the number of lines *n*, which must be less than or equal to 768. This is followed by the list of points.

#### Point lines

> 	-4.0 4.0 8.0

Each point *must* be on a separate line and *must* be indented with a tab. It *must* contain exactly 3 floating-point values in the order *x y z*. Y denotes "up".

### NORMALS

> NORMALS n

Optional. This starts a list of vertex normals with the number of lines *n*, which *must* be equal to number of polygons below (otherwise we revert to calculating normals). This is followed by the list of normal lines.

#### Normal lines

> 	-1.0 0.0 0.0 0.0 -1.0 0.0 0.0 1.0 0.0

Each line *must* be indented with a tab. It *must* contain exactly 3 normals (one per indexed vertex in a corresponding polygon line) and each normal *must* contain exactly 3 floating-point values in the *x y z* order.

### POLYGONS

> POLYGONS n

This starts a list of polygon faces with the number of lines *n*.

#### Polygon lines

> 	200 3 3 2 1 0.82 0.78 0.186 0.78 0.199 0.82

Each polygon *must* be on a separate line and *must* be indented with a tab.

> 	Flags Number\_of\_points Point\_order [Optional\_animation\_block] Texture\_coordinates

##### Flags

* +200 means the polygon is textured. Each entry in POLYGONS must have this flag.
* +4000 means the polygon has a team colour or animation frames. If it is set, an animation block is required.
* No other flags are supported. Note that if you want a surface to display something on both sides, make two polygons, one for each side.

##### Number of points

* The first number is the number of points for this polygon. Only triangles are supported, so each entry *must* use 3 points.

##### Animation block

If the flag +4000 is set, the following 4 arguments must follow:
* First, add the number of texture animation or team colour images provided. For team colour, it is always 8.
* Second, add the playback rate. The units are game ticks per frame (effectively millisecond per frame), and appear to be used exclusively for animating muzzle-flash effects. Unfortunately, the default value is typically '1' which is far too rapid to actually see the muzzle-flash animating. :(
* Third and fourth add the x and y size (width and height) of the animation frame. The x value cannot be zero -- if you want a texture animation to scroll vertically instead of horizontally, specify a width of 256.

##### Point order

* This is a list of indexes to the list of points given in the POINTS section.

##### Texture coordinates

* Give texture coordinates for each point. There are two texture floating-point coordinates for each point, hence this list should be twice as long as the number of points. The coordinate is given in UV 0.0-1.0 range.

### CONNECTORS

> CONNECTORS n

This starts a list of connectors for the model with the number of lines *n*.

Connectors are used to place and orient other components with this one.
Not every model requires them; the meaning of each connector is special and hardcoded, but generally speaking:

* PIE models representing vehicle chassis should have two connectors: Connector 1 identifies the turret location for ground vehicles, while connector 2 identifies the turret location for VTOL weaponry. (Note that VTOL weapon turrets are oriented upside-down with respect to ground turrets; this is part of the rendering process and is not actual model geometry.)
* Likewise, PIE models representing structures may have one connector indicating where to place a sensor or weapon turret.
* Turrets themselves, weapon or otherwise, are actually assembled from two separate PIE files -- one represents the base mount and one represents the muzzle. Of these two:
* The turret base mount may have one connector indicating where the turret muzzle is located, however this appears to be respected only for cyborg weaponry (where the "turret base" is the cyborg torso graphic, and the "turret muzzle" is the side-mounted weapon graphic). Turret muzzles on vehicles and structures are both located with respect to the chassis/structure connector, ignoring any connectors present on the base mount.
* Turret muzzles should have one connector on their business end to indicate where the muzzle-flash graphic is displayed while the weapon is firing.

#### Connector lines

> 	-8 -14 24

Each connector must be on a separate line and must be indented with a tab.
It contains the x, y, and z coordinates of a connector. Note that unlike in point coordinates, the Z coordinate denotes "up".

### ANIMOBJECT

> ANIMOBJECT time cycles frames

If the mesh is animated, this directive will tell the game how to animate it. The values
are the total animation time (of all frames), the number of cycles to render the animation,
where zero is infinitely many, and finally the number of animation frames that follows.

#### Animation frame lines

> 	frame xpos zpos ypos xrot zrot yrot xscale yscale zscale

(note order x,z,y order here! but not for scale where its x,y,z!)

Each animation line starts with a tab followed by

* the serially increasing frame number
* three 3-dimensional vectors, one for position, one for rotation, and one for scaling.

NOTE: For position and rotation - but not scaling - Y and Z are swapped.

If the scaling values are negative, they indicate that the animation is a legacy
keyframe animation sequence. Do not use this in future content.

### SHADOWPOINTS _(optional)_

> SHADOWPOINTS n

This starts a list of vertex coordinates (points) with the number of lines *n*. This is followed by the list of points. This (along with `SHADOWPOLYGONS`) is used to provide an alternate (simplified) shadow mesh to improve performance (recommended if the normal mesh is complex / high-poly-count). If unspecified, the normal mesh is used for shadows.

#### Point lines

> 	-4.0 4.0 8.0

Each point *must* be on a separate line and *must* be indented with a tab. It *must* contain exactly 3 floating-point values in the order *x y z*. Y denotes "up".

### SHADOWPOLYGONS _(optional)_

> POLYGONS n

This starts a list of polygon faces with the number of lines *n*. This (along with `SHADOWPOINTS`) is used to provide an alternate (simplified) shadow mesh to improve performance (recommended if the normal mesh is complex / high-poly-count). If unspecified, the normal mesh is used for shadows.

#### Polygon lines

> 	0 3 3 2 1

Each polygon *must* be on a separate line and *must* be indented with a tab.

> 	Flags Number\_of\_points Point\_order

##### Flags

* 0 means the polygon is untextured. Each entry in SHADOWPOLYGONS must have this flag.
* No other flags are supported. Note that if you want a surface to display something on both sides, make two polygons, one for each side.

##### Number of points

* The first number is the number of points for this polygon. Only triangles are supported, so each entry *must* use 3 points.

##### Point order

* This is a list of indexes to the list of points given in the POINTS section.
