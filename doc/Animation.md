Animation system
================

There are two ways to implement animation in Warzone.

Texture animation
-----------------

You can make flip through a stack of textures to implement
animation. They need to be lined up in the texture page
horizontally, and the offset and number of frames to cycle
through are specified in the model file.

See the [PIE format specification](PIE.markdown) for details.

Model animation
---------------

Currently implemented for structures and droid bodies (which includes
cyborg feet). You can specify animations that trigger on movement,
structure activation (only implemented for Power Generator so far),
shooting, and dying.

See the [PIE format specification](PIE.markdown) for details.
