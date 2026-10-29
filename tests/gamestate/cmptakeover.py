#!/usr/bin/env python3
# Checks the savegame camtakeover.sh writes after the takeover:
# NEXUS's factory must be where it was built (facing 90 degrees, covering tiles (x - 1, y) and (x, y)),
# its foundation corners at its pos.z and within the map's height range,
# and no land tile may be out of that range.
#
# Usage: cmptakeover.py <savegame folder> <site x> <site y>
# Exit status: 0 passed, 1 failed, 2 bad input.
import base64
import json
import os
import struct
import sys
import zipfile

TILE_UNITS = 128
TILE_MAX_HEIGHT = 510
TER_WATER = 7
NEXUS = 3
DIRECTION_90 = 0x4000


def find_structures(node, out):
    if isinstance(node, dict):
        if node.get('statId') == 'A0CyborgFactory' and node.get('player') == NEXUS and 'pos' in node:
            out.append(node)
        for v in node.values():
            find_structures(v, out)
    elif isinstance(node, list):
        for v in node:
            find_structures(v, out)


def main(argv):
    if len(argv) != 4:
        print('usage: cmptakeover.py <savegame folder> <site x> <site y>')
        return 2
    folder, sx, sy = argv[1], int(argv[2]), int(argv[3])
    with zipfile.ZipFile(os.path.join(folder, 'gamestate.wz')) as z:
        gs = json.loads(z.read('gamestate.json'))['gameState']
    terrain = gs['mapTerrain']
    w, h = terrain['width'], terrain['height']
    heights = struct.unpack('<%di' % (w * h), base64.b64decode(terrain['tileHeight']))
    textures = struct.unpack('<%dH' % (w * h), base64.b64decode(terrain['texture']))
    ttypes = base64.b64decode(terrain['terrainTypes'])
    failures = []

    def height(x, y):
        return heights[x + y * w]

    # Where addStructure() snapped it (2 x 1 tiles when facing 90 degrees)
    expected = (sx * TILE_UNITS, sy * TILE_UNITS + TILE_UNITS // 2)
    factories = []
    find_structures(gs['world'], factories)
    factory = next((s for s in factories if abs(s['pos'][0] - expected[0]) <= TILE_UNITS and abs(s['pos'][1] - expected[1]) <= TILE_UNITS), None)
    if factory is None:
        print('cmptakeover: NEXUS has no cyborg factory at the site (%d, %d) - its cyborg factories: %s'
              % (sx, sy, [s['pos'] for s in factories]))
        return 1
    pos, direction = factory['pos'], factory['rot'][0]
    if (pos[0], pos[1]) != expected or direction != DIRECTION_90:
        failures.append('the factory was rebuilt at (%d, %d) facing %d, but was built at (%d, %d) facing %d'
                        % (pos[0], pos[1], direction, expected[0], expected[1], DIRECTION_90))
    corners = [(x, y) for y in (sy, sy + 1) for x in (sx - 1, sx, sx + 1)]
    corner_heights = [height(x, y) for x, y in corners]
    if not 0 <= pos[2] <= TILE_MAX_HEIGHT or any(ch != pos[2] for ch in corner_heights):
        failures.append('the factory is at height %d, its foundation corners at %s' % (pos[2], corner_heights))
    # (Only water tiles are lowered to the riverbed)
    bad = [(i % w, i // w, heights[i]) for i in range(w * h)
           if ttypes[textures[i] & 0x1ff] != TER_WATER and not 0 <= heights[i] <= TILE_MAX_HEIGHT]
    if bad:
        failures.append('%d land tile(s) out of the height range, ex. %s' % (len(bad), bad[:5]))
    if failures:
        for f in failures:
            print('cmptakeover: FAILED: ' + f)
        return 1
    print('cmptakeover: the factory is where it was built, facing the same way, at height %d (foundation %s)'
          % (pos[2], sorted(set(corner_heights))))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
