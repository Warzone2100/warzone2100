def getTexAnimGrp(bm, layer):
    return bm.faces.layers.int.get(layer) or bm.faces.layers.int.new(layer)