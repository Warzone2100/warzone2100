e_PieVersion = (
    ('2', 'PIE 2', 'Legacy format.'),
    ('3', 'PIE 3', 'Stable format.'),
    ('4', 'PIE 4', 'Latest format. Allows model flags and textures to be defined for each level.')
)

e_ObjectPieType = (
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
)

e_ObjectPieShadowType = (
    ('DEFAULT', 'Default', 'The shadow will default to the level mesh'),
    ('CUSTOM', 'Custom', (
        'Define the level shadow with a PIE "Shadow" mesh object. '
        'This object must be parented to the level object'
        )),
    # ('CONVEXHULL', 'Convex Hull', (
    #     'Define the level shadow with a convex hull of the level mesh'
    #     )),
)

e_TexMapSlot = (
    ('TEXTURE', 'Base', 'Determines the basic color for the model. This texture should always be defined for textured models'),
    ('TCMASK', 'Team', 'Determines the team color mask for the model'),
    ('NORMALMAP', 'Norm', 'Determines the normal map for the model'),
    ('SPECULARMAP', 'Spec', 'Determines the specular map for the model'),
)

e_TexMapTileset = (
    ('0', 'Arizona', 'The default tileset. For any defined texture map, there should always be one defined for this tileset'),
    ('1', 'Urban', 'This texture will be used on game maps which use the Urban tileset'),
    ('2', 'Rockies', 'This texture will be used on game maps which use the Rockies tileset'),
)


def findPieRootObject(ob):
    ob_field = ob.pie_object_prop

    # check if object is already a PIE ROOT
    if ob_field.pieType == 'ROOT':
        return ob

    # if object has no parent, there is no PIE ROOT
    if ob.parent == None:
        return None

    ob_parents_remain = True
    child_ob = ob
    while ob_parents_remain:
        # getting parent of last checked object
        parent_ob = child_ob.parent
        parent_field = parent_ob.pie_object_prop

        # if parent object itself has no more parents, this is the last iteration
        if parent_ob.parent == None:
            ob_parents_remain = False

        # if parent object is the PIE ROOT, return it
        if parent_field.pieType == 'ROOT':
            return parent_ob
        else:
            # this parent was not a PIE ROOT, use this object as the child in the next iteration
            child_ob = parent_ob

    # if not PIE ROOT is found at this point, there is not a PIE ROOT for this object
    return None


def getTexAnimGrp(bm):
    layer = bm.faces.layers.int.get('pieTexAnimGrp')

    # if a face layer with that name exists, return it
    if layer:
        return layer

    # if not, create one with that name
    layer = bm.faces.layers.int.new('pieTexAnimGrp')

    # initialize values such that it can't match a potential list item index
    for face in bm.faces:
        face[layer] = -1

    return layer


def getPieUnusedTexMapSlot(ob):
    tilesetCount = len(e_TexMapTileset)
    slotTypes = list(list(zip(*e_TexMapSlot))[0])
    slots = [slotTypes.copy() for ii in range(tilesetCount)]

    tilesetIndex = 0

    for txm in ob.pie_tex_maps:
        try:  # remove slot from tileset to mark it as unavailable
            slots[int(txm.tileset)].remove(txm.slot)
        except ValueError:
            pass  # existing tex maps have a duplicate of slot

    # if all slots in this tileset are used, remove the tileset and move the tileset counter to the next
    for ii in range(tilesetCount):
        if not len(slots[0]):
            slots.pop(0)
            tilesetIndex += 1
        else:
            break

    if len(slots):
        return slotTypes.index(slots[0][0]), tilesetIndex
    else:
        return None  # no unused slots
