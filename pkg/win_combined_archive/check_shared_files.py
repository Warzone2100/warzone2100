#!/usr/bin/env python3
# Checks that an architecture archive's files outside bin/ match the x64 archive's, allowing only build
# timestamps to differ: member timestamps inside the .wz data archives, and generated docs' "Last updated" line.
#
# Usage: check_shared_files.py <x64 archive.zip> <other archive.zip>

import io
import re
import sys
import zipfile

LAST_UPDATED = re.compile(rb'^Last updated .*$', re.MULTILINE)


def shared_members(archive):
    return {info.filename: info for info in archive.infolist()
            if not info.is_dir() and not info.filename.startswith('bin/')}


def wz_contents(archive, name):
    with zipfile.ZipFile(io.BytesIO(archive.read(name))) as wz:
        return sorted((info.filename, info.CRC, info.file_size) for info in wz.infolist())


def main():
    if len(sys.argv) != 3:
        print(f'Usage: {sys.argv[0]} <x64 archive.zip> <other archive.zip>')
        return 2
    problems = []
    with zipfile.ZipFile(sys.argv[1]) as x64, zipfile.ZipFile(sys.argv[2]) as other:
        x64_members = shared_members(x64)
        other_members = shared_members(other)
        for name in sorted(set(x64_members) ^ set(other_members)):
            problems.append(f'Only in one archive: {name}')
        for name in sorted(set(x64_members) & set(other_members)):
            if x64_members[name].CRC == other_members[name].CRC:
                continue
            if name.endswith('.wz'):
                if wz_contents(x64, name) == wz_contents(other, name):
                    continue
            elif LAST_UPDATED.sub(b'', x64.read(name)) == LAST_UPDATED.sub(b'', other.read(name)):
                continue
            problems.append(f'Differs: {name}')
    for problem in problems:
        print(problem)
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
