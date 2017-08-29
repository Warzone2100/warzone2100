PIE 2
=====

Description
-----------

The PIE format is a custom model format created by Pumpkin.

It has gone through two iterations since the original commercial release. PIE2 was
used until version 2.3 and is still supported. PIE3 is the latest version and is
described here.

The big change between PIE2 and PIE3 is that values are floating point in the latter,
whereas values in PIE2 are scaled up 256 times.

Format
------

### PIE

> PIE 3

The first line specifies the version number (3) and MUST read like that.

### TYPE

> TYPE X

This indicates the type of the file through a hexadecimal combination of flags. The following values can be used:
 * 0x00200  - Reserved for backward compatibility.
 * 0x01000  - Specifies that the model should not be stretched to fit terrain. For defensive buildings that have a deep foundation.
 * 0x10000  - Specifies the usage of TCMask feature (Replacement for the old team coloration methods. Check ticket #851).

### TEXTURE

> TEXTURE 0 page-7-barbarians-arizona.png 0 0

This sets the texture page for the model. The second value gives you the filename of the texture page, which must end with ".png". The PIE file MUST contain exactly one TEXTURE line. The filename MUST only contain [a-zA-Z0-9._\-]. The file name SHOULD start with "page-NN-" for correct handling of dynamic texture replacement. The first, third and fourth values are ignored, and should be zero.

### NORMALMAP

> NORMALMAP 0 page-7-barbarians-arizona_normal.png 0 0

Optional. As above, but this sets the normal map texture page for the model.

### SPECULARMAP

> SPECULARMAP 0 page-7-barbarians-arizona.png 0 0

Optional. As above, but this sets the specular map texture page for the model.

### EVENT

> EVENT type filename.pie

An animation event associated with this model. If the event type is triggered, the model is
replaced with the specified model for the duration of the event. The following event types are defined:

  * 1 - Active event. What this means depends on the type of model. For droids this means movement,
    while for power generators it means they are linked to a power source.
  * 2 - Firing. The model is firing at some enemy.
  * 3 - Dying. The model is dying. You (almost) always want to make sure animation cycles for this model is set to 1
    for the specified model - if it is zero, it will never die!

### LEVELS

> LEVELS 1

This gives the number of meshes that are contained in this model. Each mesh can be animated separately in ANI files.

### LEVEL

> LEVEL 1

This starts the model description for mesh 1. Repeat the below as necessary while incrementing the value above as needed.

### SHADERS

> SHADERS 2 vertex.vert fragment.vert

Optional. Create a specific shader program for this mesh. The number '2' is not parsed but should always be '2'.

### POINTS

> POINTS n

This starts a list of vertex coordinates (points) that is ''n'' lines long. ''n'' MUST be less than or equal to 768. This is followed by the list of points.

#### Point

> -4.0 4.0 8.0

Each point MUST be on a separate line and MUST be indented with a tab. It MUST contain exactly 3 floating-point values in the order ''x y z''. Y denotes "up".

### POLYGONS

> POLYGONS n

This starts a list of polygon faces. ''n'' MUST be less than or equal to 512.

#### Polygon

> 200 3 3 2 1 0.82 0.78 0.186 0.78 0.199 0.82

Each polygon MUST be on a separate line and MUST be indented with a tab.

It is made out of following sections:

> Flags Points Texture_coordinates

''' Flags '''
 * +200 means the polygon is textured. Each entry in POLYGONS MUST have this flag.
 * No other flags are supported. Note that if you want a surface to display something on both sides, make two polygons, one for each side.

''' Points '''
 * First number is the number of points for this polygon. Each entry MUST be between 3 and 6 points, however using triangles only is STRONGLY RECOMMENDED. At some later point, only triangles will be supported, and other polygons will be tessellated.
 * Then follows a list of indexes to the points list.

''' Texture coordinates '''
 * Give texture coordinates for each point. There are two texture floating-point coordinates for each point, hence this list should be twice as long as the number of points. The coordinate is given in UV 0.0-1.0 range.

### CONNECTORS

> CONNECTORS n

This starts a list of connectors for the model. These are used to place other components against this one. For each line following this, you should indent by a tab, then give the x, y, and z coordinates of a connector. Z denotes "up." The meaning of each connector is special and hard-coded. Some models do not need connectors.

The exact purpose of each connector is hard-coded for each model type. TODO: List these purposes here.

### ANIMOBJECT

> ANIMOBJECT time cycles frames

If the mesh is animated, this directive will tell the game how to animate it. The values
are the total animation time (of all frames), the number of cycles to render the animation,
where zero is infinitely many, and finally the number of animation frames that follows.

#### Animation frame

> frame xpos ypos zpos xrot yrot zrot xscale yscale zscale

Each animation line starts with the serially increasing frame number, followed by
three (x, y, z) vectors, one for position, one for rotation, and one for scaling.

If the scaling values are negative, they indicate that the animation is a legacy
keyframe animation sequence. Do not use this in future content.
