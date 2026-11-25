#!/usr/bin/env python3
# Packs a test mod directory into a .wz archive (a zip whose paths are relative to the directory).
#
# Usage: mkmod.py <mod directory> <archive>
#
# ex. mkmod.py "$MOD" "$CFG/mods/global/builtins.wz"
import os
import sys
import zipfile


def main():
    if len(sys.argv) != 3:
        sys.exit("usage: mkmod.py <mod directory> <archive>")
    root, dest = sys.argv[1], sys.argv[2]
    with zipfile.ZipFile(dest, "w") as z:
        for d, dirs, files in os.walk(root):
            dirs.sort()
            for f in sorted(files):
                path = os.path.join(d, f)
                z.write(path, os.path.relpath(path, root))


if __name__ == "__main__":
    main()
