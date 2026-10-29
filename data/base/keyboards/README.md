# On-screen keyboard layouts

These files define the character layouts used by the built-in on-screen
keyboard (`lib/widget/virtualkeyboard.cpp`).

**Do not edit the `.json` files by hand.** They are generated from
[xkeyboard-config](https://gitlab.freedesktop.org/xkeyboard-config/xkeyboard-config)
by `tools/keyboard-layouts/generate_layouts.py`, which compiles each layout
with libxkbcommon and records the characters every physical key produces.

| File      | Layout                  | xkeyboard-config source |
|-----------|-------------------------|-------------------------|
| `us.json` | English (US, QWERTY)    | `symbols/us`            |
| `de.json` | Deutsch (QWERTZ)        | `symbols/de`            |
| `fr.json` | Français (AZERTY)       | `symbols/fr`            |
| `ru.json` | Русский (ЙЦУКЕН)        | `symbols/ru`            |

## Format

- `keys` maps XKB key names (`TLDE`, `AE01`–`AE12`, `AD01`–`AD12`, `BKSL`,
  `AC01`–`AC11`, `LSGT`, `AB01`–`AB10`) to up to four shift levels:
  base, Shift, AltGr, AltGr+Shift.
- A level is either a string (the character the key types) or a dead key,
  `{"dead": "<combining mark>", "label": "<display character>"}`, which
  combines with the next character typed.
- `geometry` is `ansi` (backslash key at the end of the top letter row) or
  `iso` (backslash key at the end of the home row, extra key before Z).

## Accents (`accents.json`)

Holding a letter key on the on-screen keyboard offers accented and related
letters. `accents.json` lists them per game language, from the
[Unicode CLDR](https://cldr.unicode.org) exemplar characters (the main set,
then the auxiliary set): `languages` maps a language code (as the game
reports it, ex. `de` or `pt_BR`) to the plain letters and the letters offered
on them, ex. `"e": ["é", "è", "ê", "ë"]`. Letters are grouped by their
canonical decomposition, or by a related letter for those that don't
decompose (`æ` on `a`, `ß` on `s`, Ukrainian `є` on `е`). Only precomposed
Latin, Cyrillic and Greek letters are included. The game offers the
letters of the current game language, then those of the layout's language.
Uppercase variants are derived at runtime.

## Regenerating

```sh
python3 tools/keyboard-layouts/generate_layouts.py
```

This needs libxkbcommon and an xkeyboard-config data tree (by default
`/usr/share/X11/xkb`, or pass `--xkb-root` for another). The version is read
from the installed `xkeyboard-config.pc`, or pass `--xkb-version`. Add
layouts by extending `LAYOUTS` in the script.

```sh
npm pack cldr-misc-full@48.2.0 && tar xzf cldr-misc-full-48.2.0.tgz
python3 tools/keyboard-layouts/generate_accents.py --cldr package
```

This regenerates `accents.json` from the JSON form of CLDR
([cldr-json](https://github.com/unicode-org/cldr-json)). Add languages by
extending `LANGUAGES` in the script.

## License

The generated layout data is derived from xkeyboard-config, which is
distributed under the permissive licenses reproduced in
`COPYING.xkeyboard-config`.

`accents.json` is derived from Unicode CLDR, distributed under the Unicode
License v3 reproduced in `COPYING.unicode`.
