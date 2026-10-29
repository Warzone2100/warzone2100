# Keyboard layout generator

`generate_layouts.py` produces the on-screen keyboard layout data in
`data/base/keyboards/` from [xkeyboard-config](https://gitlab.freedesktop.org/xkeyboard-config/xkeyboard-config),
using libxkbcommon (through `ctypes`, so there are no Python dependencies).

```sh
python3 tools/keyboard-layouts/generate_layouts.py [--xkb-root DIR] [--xkb-version VER] [--out DIR]
```

- `--xkb-root`: the xkeyboard-config data tree (default `/usr/share/X11/xkb`).
  The keymaps are compiled directly from the `keycodes`, `types`, `compat` and
  `symbols` components, so an unbuilt upstream source tree works as well
  (it doesn't need the generated `rules` files).
- `--xkb-version`: recorded in each output file. By default it is read from the
  installed `xkeyboard-config.pc`.

The committed data was generated from xkeyboard-config 2.41. See
`data/base/keyboards/README.md` for the file format and license details.
