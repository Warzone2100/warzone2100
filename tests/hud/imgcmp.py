#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later
#
# This file is part of Warzone 2100.
# Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)
#
# Warzone 2100 is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Additional terms under Section 7 of the GNU General Public License
# version 3 apply to this file: version 1.0 of the Warzone 2100
# additional terms, or (at your option) any later version, as published
# by the Warzone 2100 Project. They are provided in COPYING.SECTION7 in
# the top-level directory of the source distribution. This notice must
# be kept intact.
#
# Warzone 2100 is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.

"""Compares the screenshots of two HUD geometry dumps (tests/hud/geometry.sh), state by state, and prints for each
IDENTICAL, or how many pixels differ and their bounding box. PNGs are read with Pillow when it's installed, and with a
small built-in decoder otherwise.

Exit status: 0 every screenshot is within the limits (by default, identical), 1 one isn't, 2 bad input.
"""

import argparse
import os
import struct
import sys
import zlib


def read_png_builtin(path):
    with open(path, "rb") as f:
        data = f.read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path}: not a PNG")
    pos = 8
    idat = []
    width = height = depth = ctype = interlace = None
    while pos < len(data):
        length, kind = struct.unpack(">I4s", data[pos:pos + 8])
        chunk = data[pos + 8:pos + 8 + length]
        if kind == b"IHDR":
            width, height, depth, ctype, _, _, interlace = struct.unpack(">IIBBBBB", chunk)
        elif kind == b"IDAT":
            idat.append(chunk)
        elif kind == b"IEND":
            break
        pos += 12 + length
    if depth != 8 or ctype not in (2, 6) or interlace:
        raise ValueError(f"{path}: unsupported PNG format")
    bpp = 3 if ctype == 2 else 4
    raw = zlib.decompress(b"".join(idat))
    stride = width * bpp
    out = bytearray(height * stride)
    prev = bytearray(stride)
    pos = 0
    for y in range(height):
        ftype = raw[pos]
        line = bytearray(raw[pos + 1:pos + 1 + stride])
        pos += 1 + stride
        if ftype == 1:
            for i in range(bpp, stride):
                line[i] = (line[i] + line[i - bpp]) & 0xFF
        elif ftype == 2:
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 0xFF
        elif ftype == 3:
            for i in range(stride):
                left = line[i - bpp] if i >= bpp else 0
                line[i] = (line[i] + ((left + prev[i]) >> 1)) & 0xFF
        elif ftype == 4:
            for i in range(stride):
                a = line[i - bpp] if i >= bpp else 0
                b = prev[i]
                c = prev[i - bpp] if i >= bpp else 0
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pred = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                line[i] = (line[i] + pred) & 0xFF
        out[y * stride:(y + 1) * stride] = line
        prev = line
    if bpp == 4:
        rgb = bytearray(width * height * 3)
        rgb[0::3] = out[0::4]
        rgb[1::3] = out[1::4]
        rgb[2::3] = out[2::4]
        out = rgb
    return width, height, bytes(out)


def read_png(path):
    try:
        from PIL import Image
    except ImportError:
        return read_png_builtin(path)
    with Image.open(path) as img:
        img = img.convert("RGB")
        return img.width, img.height, img.tobytes()


def compare(ref_path, cand_path, tolerance):
    rw, rh, rpix = read_png(ref_path)
    cw, ch, cpix = read_png(cand_path)
    if (rw, rh) != (cw, ch):
        return None, f"size {rw}x{rh} vs {cw}x{ch}"
    if rpix == cpix:
        return 0, None
    count = 0
    x0, y0, x1, y1 = rw, rh, -1, -1
    stride = rw * 3
    for y in range(rh):
        row_r = rpix[y * stride:(y + 1) * stride]
        row_c = cpix[y * stride:(y + 1) * stride]
        if row_r == row_c:
            continue
        for x in range(rw):
            i = x * 3
            if max(abs(row_r[i] - row_c[i]), abs(row_r[i + 1] - row_c[i + 1]), abs(row_r[i + 2] - row_c[i + 2])) > tolerance:
                count += 1
                x0, y0, x1, y1 = min(x0, x), min(y0, y), max(x1, x), max(y1, y)
    return count, (None if count == 0 else f"box {x0},{y0} to {x1},{y1}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("reference", help="the reference screenshots' directory")
    parser.add_argument("candidate", help="the candidate screenshots' directory")
    parser.add_argument("--tolerance", type=int, default=0, help="the largest channel difference that counts as the same")
    parser.add_argument("--max-pixels", type=int, default=0, help="how many pixels may differ")
    args = parser.parse_args()
    ref_dir, cand_dir, tolerance, max_pixels = args.reference, args.candidate, args.tolerance, args.max_pixels
    names = sorted(n for n in os.listdir(ref_dir) if n.endswith(".png"))
    if not names:
        print(f"no screenshots in {ref_dir}")
        return 2
    failed = False
    for name in names:
        cand = os.path.join(cand_dir, name)
        if not os.path.exists(cand):
            print(f"  {name}: MISSING")
            failed = True
            continue
        count, detail = compare(os.path.join(ref_dir, name), cand, tolerance)
        if count is None:
            print(f"  {name}: {detail}")
            failed = True
        elif count == 0:
            print(f"  {name}: IDENTICAL")
        else:
            ok = count <= max_pixels
            failed = failed or not ok
            print(f"  {name}: {count} pixel(s) differ ({detail}){'' if ok else ' FAILED'}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
