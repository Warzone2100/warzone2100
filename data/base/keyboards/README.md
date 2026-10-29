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

## Regenerating

```sh
python3 tools/keyboard-layouts/generate_layouts.py
```

This needs libxkbcommon and an xkeyboard-config data tree (by default
`/usr/share/X11/xkb`, or pass `--xkb-root` for another). The version is read
from the installed `xkeyboard-config.pc`, or pass `--xkb-version`. Add
layouts by extending `LAYOUTS` in the script.

## License

The generated data is derived from xkeyboard-config, which is distributed
under the permissive licenses reproduced in `COPYING.xkeyboard-config`.
