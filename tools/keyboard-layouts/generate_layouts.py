#!/usr/bin/env python3
#
# This file is part of Warzone 2100.
# Copyright (C) 2026  Warzone 2100 Project
#
# Warzone 2100 is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Warzone 2100 is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Warzone 2100; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#
"""Generate on-screen keyboard layout data from xkeyboard-config.

Compiles each layout with libxkbcommon (from the xkeyboard-config component
files - no rules database needed) and writes one JSON file per layout, listing
the characters each physical key produces on its first four shift levels
(base, Shift, AltGr, AltGr+Shift). Dead keys are emitted as their combining
mark plus a display label.

The output is committed to data/base/keyboards/ - the game never needs
libxkbcommon or xkeyboard-config at runtime.

Requires libxkbcommon (shared library) and an xkeyboard-config data tree.

Usage:
    generate_layouts.py [--xkb-root DIR] [--xkb-version VER] [--out DIR]
"""

import argparse
import ctypes
import ctypes.util
import json
import os
import sys

# The layouts to generate: output id -> settings.
#  geometry: "ansi" or "iso" - where the backslash key sits, and whether the
#            extra ISO key (LSGT, between left Shift and Z) exists
#  name:     native display name shown in the keyboard and the options
LAYOUTS = {
    "us": {"layout": "us", "variant": "", "geometry": "ansi", "name": "English (US, QWERTY)"},
    "de": {"layout": "de", "variant": "", "geometry": "iso", "name": "Deutsch (QWERTZ)"},
    "fr": {"layout": "fr", "variant": "", "geometry": "iso", "name": "Français (AZERTY)"},
    "ru": {"layout": "ru", "variant": "", "geometry": "ansi", "name": "Русский (ЙЦУКЕН)"},
}

# The physical keys the on-screen keyboard shows, by XKB key name
KEY_NAMES = (
    ["TLDE"] + ["AE%02d" % i for i in range(1, 13)]
    + ["AD%02d" % i for i in range(1, 13)] + ["BKSL"]
    + ["AC%02d" % i for i in range(1, 12)]
    + ["LSGT"] + ["AB%02d" % i for i in range(1, 11)]
)

# Dead keysym name -> (combining mark, spacing display label)
DEAD_KEYS = {
    "dead_grave": ("̀", "`"),
    "dead_acute": ("́", "´"),
    "dead_circumflex": ("̂", "^"),
    "dead_tilde": ("̃", "~"),
    "dead_perispomeni": ("͂", "~"),
    "dead_macron": ("̄", "¯"),
    "dead_breve": ("̆", "˘"),
    "dead_abovedot": ("̇", "˙"),
    "dead_diaeresis": ("̈", "¨"),
    "dead_abovering": ("̊", "˚"),
    "dead_doubleacute": ("̋", "˝"),
    "dead_caron": ("̌", "ˇ"),
    "dead_cedilla": ("̧", "¸"),
    "dead_ogonek": ("̨", "˛"),
}

NUM_LEVELS = 4


class Xkb:
    def __init__(self):
        path = ctypes.util.find_library("xkbcommon")
        if not path:
            sys.exit("error: libxkbcommon not found (install libxkbcommon)")
        lib = ctypes.CDLL(path)
        lib.xkb_context_new.restype = ctypes.c_void_p
        lib.xkb_context_new.argtypes = [ctypes.c_int]
        lib.xkb_context_include_path_append.restype = ctypes.c_int
        lib.xkb_context_include_path_append.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        lib.xkb_context_unref.argtypes = [ctypes.c_void_p]
        lib.xkb_keymap_new_from_string.restype = ctypes.c_void_p
        lib.xkb_keymap_new_from_string.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_int]
        lib.xkb_keymap_unref.argtypes = [ctypes.c_void_p]
        lib.xkb_keymap_key_by_name.restype = ctypes.c_uint32
        lib.xkb_keymap_key_by_name.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        lib.xkb_keymap_num_levels_for_key.restype = ctypes.c_uint32
        lib.xkb_keymap_num_levels_for_key.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32]
        lib.xkb_keymap_key_get_syms_by_level.restype = ctypes.c_int
        lib.xkb_keymap_key_get_syms_by_level.argtypes = [
            ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint32,
            ctypes.POINTER(ctypes.POINTER(ctypes.c_uint32))]
        lib.xkb_keysym_to_utf32.restype = ctypes.c_uint32
        lib.xkb_keysym_to_utf32.argtypes = [ctypes.c_uint32]
        lib.xkb_keysym_get_name.restype = ctypes.c_int
        lib.xkb_keysym_get_name.argtypes = [ctypes.c_uint32, ctypes.c_char_p, ctypes.c_size_t]
        self.lib = lib

    def context(self, xkb_root):
        XKB_CONTEXT_NO_DEFAULT_INCLUDES = 1
        XKB_CONTEXT_NO_ENVIRONMENT_NAMES = 2
        ctx = self.lib.xkb_context_new(XKB_CONTEXT_NO_DEFAULT_INCLUDES | XKB_CONTEXT_NO_ENVIRONMENT_NAMES)
        if not ctx or not self.lib.xkb_context_include_path_append(ctx, xkb_root.encode()):
            sys.exit("error: could not create an XKB context for %s" % xkb_root)
        return ctx

    def keysym_name(self, sym):
        buf = ctypes.create_string_buffer(64)
        self.lib.xkb_keysym_get_name(sym, buf, len(buf))
        return buf.value.decode()


def compile_keymap(xkb, ctx, layout, variant):
    symbols = "pc+%s%s+inet(evdev)" % (layout, "(%s)" % variant if variant else "")
    keymap_src = (
        'xkb_keymap {\n'
        '  xkb_keycodes { include "evdev+aliases(qwerty)" };\n'
        '  xkb_types { include "complete" };\n'
        '  xkb_compat { include "complete" };\n'
        '  xkb_symbols { include "%s" };\n'
        '};\n' % symbols)
    XKB_KEYMAP_FORMAT_TEXT_V1 = 1
    keymap = xkb.lib.xkb_keymap_new_from_string(ctx, keymap_src.encode(), XKB_KEYMAP_FORMAT_TEXT_V1, 0)
    if not keymap:
        sys.exit("error: could not compile keymap for %s" % symbols)
    return keymap


def key_levels(xkb, keymap, name):
    keycode = xkb.lib.xkb_keymap_key_by_name(keymap, name.encode())
    if keycode == 0:
        return None
    num_levels = xkb.lib.xkb_keymap_num_levels_for_key(keymap, keycode, 0)
    levels = []
    for level in range(NUM_LEVELS):
        value = ""
        if level < num_levels:
            syms = ctypes.POINTER(ctypes.c_uint32)()
            count = xkb.lib.xkb_keymap_key_get_syms_by_level(keymap, keycode, 0, level, ctypes.byref(syms))
            if count == 1:
                sym = syms[0]
                sym_name = xkb.keysym_name(sym)
                if sym_name in DEAD_KEYS:
                    mark, label = DEAD_KEYS[sym_name]
                    value = {"dead": mark, "label": label}
                else:
                    cp = xkb.lib.xkb_keysym_to_utf32(sym)
                    # only printable characters belong on the keyboard
                    if cp >= 0x20 and cp != 0x7f:
                        value = chr(cp)
        levels.append(value)
    while levels and levels[-1] == "":
        levels.pop()
    return levels


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--xkb-root", default="/usr/share/X11/xkb", help="xkeyboard-config data tree (default: %(default)s)")
    parser.add_argument("--xkb-version", default=None, help="xkeyboard-config version, recorded in the output (default: from pkg-config data, if found)")
    parser.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "data", "base", "keyboards"), help="output directory")
    args = parser.parse_args()

    version = args.xkb_version
    if version is None:
        for pc in ("/usr/share/pkgconfig/xkeyboard-config.pc", "/usr/lib/pkgconfig/xkeyboard-config.pc"):
            if os.path.exists(pc):
                with open(pc, encoding="utf-8") as f:
                    for line in f:
                        if line.startswith("Version:"):
                            version = line.split(":", 1)[1].strip()
                break
    if not version:
        sys.exit("error: could not determine the xkeyboard-config version - pass --xkb-version")

    xkb = Xkb()
    ctx = xkb.context(args.xkb_root)
    os.makedirs(args.out, exist_ok=True)

    for layout_id, settings in LAYOUTS.items():
        keymap = compile_keymap(xkb, ctx, settings["layout"], settings["variant"])
        keys = {}
        for name in KEY_NAMES:
            if name == "LSGT" and settings["geometry"] != "iso":
                continue
            levels = key_levels(xkb, keymap, name)
            if levels:
                keys[name] = levels
        xkb.lib.xkb_keymap_unref(keymap)

        data = {
            "format": 1,
            "id": layout_id,
            "name": settings["name"],
            "geometry": settings["geometry"],
            "source": {
                "generator": "tools/keyboard-layouts/generate_layouts.py",
                "xkeyboard-config": version,
                "layout": settings["layout"],
                "variant": settings["variant"],
            },
            "keys": keys,
        }
        out_path = os.path.join(args.out, layout_id + ".json")
        with open(out_path, "w", encoding="utf-8", newline="\n") as f:
            json.dump(data, f, ensure_ascii=False, indent="\t")
            f.write("\n")
        print("wrote %s (%d keys)" % (out_path, len(keys)))

    xkb.lib.xkb_context_unref(ctx)


if __name__ == "__main__":
    main()
