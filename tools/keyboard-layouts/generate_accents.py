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
"""Generate the on-screen keyboard's long-press accent data from Unicode CLDR.

For each of the game's languages, reads the CLDR exemplar characters (the
main set, then the auxiliary set) and groups the letters by the plain letter
they're typed from: the first character of their canonical decomposition
(e -> é è ê ë ...), or a related letter for those that don't decompose
(a -> æ, s -> ß, ...). Holding a key on the on-screen keyboard offers its
letter's group. Only precomposed Latin, Cyrillic and Greek letters are
offered.

Reads the JSON form of CLDR, as published in the cldr-misc-full npm package
(https://github.com/unicode-org/cldr-json), ex.:

    npm pack cldr-misc-full@48.2.0 && tar xzf cldr-misc-full-48.2.0.tgz

The output is committed to data/base/keyboards/accents.json - the game needs
no CLDR data at runtime.

Usage:
    generate_accents.py --cldr DIR [--out FILE]

DIR is the package directory (containing package.json and main/).
"""

import argparse
import json
import os
import sys
import unicodedata

# Game language code (as getLanguage() reports it) -> CLDR locale.
# The game falls back from a language_REGION code to the language alone.
LANGUAGES = {
    "ar": "ar",
    "bg": "bg",
    "ca": "ca",
    "cs": "cs",
    "da": "da",
    "de": "de",
    "el": "el",
    "en": "en",
    "eo": "eo",
    "es": "es",
    "et": "et",
    "fa": "fa",
    "fi": "fi",
    "fr": "fr",
    "fy": "fy",
    "ga": "ga",
    "he": "he",
    "hr": "hr",
    "hu": "hu",
    "id": "id",
    "it": "it",
    "ja": "ja",
    "ko": "ko",
    "la": "la",
    "lt": "lt",
    "my": "my",
    "nb": "nb",
    "nl": "nl",
    "pl": "pl",
    "pt": "pt-PT",     # the game's "pt" is European Portuguese ...
    "pt_BR": "pt",     # ... while CLDR's "pt" is Brazilian
    "ro": "ro",
    "ru": "ru",
    "sk": "sk",
    "sl": "sl",
    "sr": "sr-Latn",   # the game's Serbian translation is in Latin script
    "tr": "tr",
    "tt": "tt",
    "uk": "uk",
    "zh": "zh",
}

# Letters with no canonical decomposition, and the key they're offered on.
# Cyrillic letters missing from the Russian layout are offered on the Russian
# letter they're closest to (ex. Ukrainian є on е, Tatar ә on а).
RELATED_LETTERS = {
    # Latin
    "æ": "a", "ß": "s", "ſ": "s", "œ": "o", "ø": "o", "ł": "l", "đ": "d",
    "ð": "d", "þ": "t", "ı": "i", "ĳ": "i", "ŋ": "n", "ŧ": "t", "ĸ": "k",
    # Cyrillic
    "ґ": "г", "є": "е", "і": "и", "ї": "и", "ђ": "д", "ј": "й", "љ": "л",
    "њ": "н", "ћ": "т", "џ": "ц", "ә": "а", "ө": "о", "ү": "у", "җ": "ж",
    "ң": "н", "һ": "х",
}

EXEMPLAR_SETS = ("exemplarCharacters", "auxiliary")

# Unicode character name prefixes of the scripts offered
SCRIPTS = ("LATIN ", "CYRILLIC ", "GREEK ")

# at most this many variants per letter (the main set comes first)
MAX_VARIANTS = 16


def parse_unicode_set(text):
    """Parses a CLDR exemplar set (UnicodeSet syntax: "[a b-d {ch} \\u00E9]")
    into its strings, in order."""
    text = text.strip()
    if not (text.startswith("[") and text.endswith("]")):
        raise ValueError("not a UnicodeSet: %r" % text)
    body = text[1:-1]
    items = []
    i = 0

    def read_char(pos):
        if body[pos] == "\\":
            if body[pos + 1] == "u":
                return chr(int(body[pos + 2:pos + 6], 16)), pos + 6
            if body[pos + 1] == "U":
                return chr(int(body[pos + 2:pos + 10], 16)), pos + 10
            return body[pos + 1], pos + 2
        return body[pos], pos + 1

    while i < len(body):
        ch = body[i]
        if ch.isspace():
            i += 1
            continue
        if ch == "{":
            end = body.index("}", i)
            items.append(body[i + 1:end])
            i = end + 1
            continue
        first, i = read_char(i)
        if i < len(body) and body[i] == "-" and i + 1 < len(body) and not body[i + 1].isspace():
            last, i = read_char(i + 1)
            items.extend(chr(c) for c in range(ord(first), ord(last) + 1))
        else:
            items.append(first)
    return items


def is_single_letter(text):
    """One precomposed letter (ex. "é", but not Russian "а́", which needs a separate combining mark)."""
    composed = unicodedata.normalize("NFC", text)
    if len(composed) != 1 or not unicodedata.category(composed).startswith("L"):
        return False
    # scripts the keyboard's layouts can have
    return unicodedata.name(composed, "").startswith(SCRIPTS)


def base_letter(text):
    """The plain letter a variant is offered on, or None."""
    nfc = unicodedata.normalize("NFC", text)
    if nfc in RELATED_LETTERS:
        return RELATED_LETTERS[nfc]
    decomposed = unicodedata.normalize("NFD", nfc)
    base = decomposed[0]
    if len(decomposed) == 1:
        return None  # a plain letter of its own
    return RELATED_LETTERS.get(base, base)


def accent_groups(characters):
    groups = {}
    for set_name in EXEMPLAR_SETS:
        if set_name not in characters:
            continue
        for item in parse_unicode_set(characters[set_name]):
            if not is_single_letter(item):
                continue  # digraphs such as {ch}, punctuation
            variant = unicodedata.normalize("NFC", item)
            base = base_letter(variant)
            if base is None or base == variant:
                continue
            group = groups.setdefault(base, [])
            if variant not in group and len(group) < MAX_VARIANTS:
                group.append(variant)
    return dict(sorted(groups.items()))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--cldr", required=True, help="the cldr-misc-full package directory")
    parser.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "data", "base", "keyboards", "accents.json"))
    args = parser.parse_args()

    with open(os.path.join(args.cldr, "package.json"), encoding="utf-8") as f:
        cldr_version = json.load(f)["version"]

    languages = {}
    for language, locale in sorted(LANGUAGES.items()):
        path = os.path.join(args.cldr, "main", locale, "characters.json")
        if not os.path.exists(path):
            print("warning: no CLDR data for %s (%s)" % (language, locale), file=sys.stderr)
            continue
        with open(path, encoding="utf-8") as f:
            characters = json.load(f)["main"][locale]["characters"]
        groups = accent_groups(characters)
        if groups:
            languages[language] = groups

    output = {
        "format": 1,
        "source": {
            "generator": "tools/keyboard-layouts/generate_accents.py",
            "cldr": cldr_version,
            "sets": list(EXEMPLAR_SETS),
        },
        "languages": languages,
    }
    with open(args.out, "w", encoding="utf-8", newline="\n") as f:
        json.dump(output, f, ensure_ascii=False, indent=1)
        f.write("\n")
    print("wrote %s (%d languages)" % (os.path.normpath(args.out), len(languages)))


if __name__ == "__main__":
    main()
